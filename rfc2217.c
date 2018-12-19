/*
 * ---------
 * rfc2217.c
 * ---------
 * Parses the RFC2217 commands and generates                   
 * respective responses to be sent to the client application.                    
 *									        
 * Project Name 	: MCS8140-SerialServer-2.1 						
 * File Revision	: 1.1								
 * Author	: From version 2.0 								
 * Date Created	: Thu Apr 3 2018						
 *
 * Modification History:
 *	 	Revision-Id	By	        Date		 Description	
 * $Id: rfc2217.c,v 1.1 2018/06/28 13:59:08 sami Exp $
 *										
 */

#include "data_types.h"
#include "rfc2217.h"
#include "serial.h"
#include "config.h"

#ifdef MCS9950_SS

#include "debug.h"
#include "99xx.h"
extern struct uart_99xx_port serial99xx_ports[UART99xx_NR];

#endif

/* Effective status for IAC escaping and interpretation */
IACState IACEscape = IACNormal;

/* Same as above during signature reception */
IACState IACSigEscape;

/* Current IAC command begin received */
static u8_t IACCommand[TmpStrLen];

/* Position of insertion into IACCommand[] */
static u8_t IACPos;

/* Modem state mask set by the client */
static u8_t ModemStateMask = ((u8_t) 255);

/* Line state mask set by the client */
static u8_t LineStateMask = ((u8_t) 0);

/* Current status of the line control lines */
//static u8_t LineState = ((u8_t) 0);
/* Current status of the modem control lines */
//static u8_t ModemState = ((u8_t) 0);
/* Break state flag */
Boolean BreakSignaled = False;
/* Input flow control flag */
Boolean InputFlow = True;
/* Cisco IOS bug compatibility */
Boolean CiscoIOSCompatible = False;
/* Device file descriptor */
int DeviceFd;

/* Com Port Control enabled flag */
Boolean TCPCEnabled = False;

/* True after retrieving the initial settings from the serial port */
Boolean InitPortRetrieved = False;

/* Initial serial port settings */
//struct termios InitialPortSettings;

/* Buffer to Network from Device */
int PortFd;
int PortNum;


/* Telnet State Machine */

struct _tnstate  tnstate_ptr[256];

/*Signature string*/
char   fw_version[1];
char   SigStr[] = "Soc Project:SerialOverIP";

 /*
 * Add a byte to a buffer
 */
void
AddToBuffer (BufferType * B, u8_t C)
{
  B->Buffer[B->WrPos] = C;
  B->WrPos = (B->WrPos + 1) % BufferSize;
}

 /*
 * Send a string to SockFd performing IAC escaping 
 */
 void
SendStr (BufferType * B, char *Str)
{
  u8_t I;
  u8_t L;
  
  L = strlen (Str);
  
  for (I = 0; I < L; I++)
    EscWriteChar (B, (u8_t) Str[I]);
}

 /*
 * Send the CPC command Command using Parm as parameter 
 */
void
SendCPCByteCommand (BufferType *ToNetBuf,u8_t Command, u8_t Parm)
{
  AddToBuffer (ToNetBuf, TNIAC);
  AddToBuffer (ToNetBuf, TNSB);
  AddToBuffer (ToNetBuf, TNCOM_PORT_OPTION);
  AddToBuffer (ToNetBuf, Command);
  EscWriteChar (ToNetBuf, Parm);
  AddToBuffer (ToNetBuf, TNIAC);
  AddToBuffer (ToNetBuf, TNSE);
}

 /* 
 * Write a char to socket performing IAC escaping
 */

void
EscWriteChar (BufferType * B, u8_t C)
{
  /* Last received byte */
  static u8_t Last = 0;
  
  if (C == TNIAC)
    AddToBuffer (B, C);
  else if (C != 0x0A && !tnstate_ptr[TN_TRANSMIT_BINARY].is_will && Last == 0x0D)
    AddToBuffer (B, 0x00);
  // thought that else keyword  was missing so added else ( sep 2 07...)
  else
    AddToBuffer (B, C);
  
  /* Set last received byte */
  Last = C;
}

 /* 
 * Send the baud rate BR to Buffer 
 */

void
SendBaudRate (BufferType *ToNetBuf,unsigned long int BR)
{
  u8_t *p;
  unsigned long int NBR;
  int i;
  
  NBR =  htonl(BR);//HTONL (BR);
  
  AddToBuffer (ToNetBuf, TNIAC);
  AddToBuffer (ToNetBuf, TNSB);
  AddToBuffer (ToNetBuf, TNCOM_PORT_OPTION);
  AddToBuffer (ToNetBuf, TNASC_SET_BAUDRATE);
  
  if (BR == 1)      /*115200 baud rate */
    {
      EscWriteChar (ToNetBuf, 0);
      EscWriteChar (ToNetBuf, 1);
      EscWriteChar (ToNetBuf, 0xc2);
      EscWriteChar (ToNetBuf, 0);
    }
  else if (BR == 2)   /*230400 baud rate */
    {
      EscWriteChar (ToNetBuf, 0);
      EscWriteChar (ToNetBuf, 0x03);
      EscWriteChar (ToNetBuf, 0x84);
      EscWriteChar (ToNetBuf, 0);
      
    }
  else
    {
      p = (u8_t *) & NBR;
      for (i = 0; i < (int) sizeof (NBR); i++)
	EscWriteChar (ToNetBuf, p[i]);
    }
  AddToBuffer (ToNetBuf, TNIAC);
  AddToBuffer (ToNetBuf, TNSE);
}


/*
* Handling of COM Port Control specific commands 
*/

void
HandleCPCCommand (BufferType * ToNetBuf,unsigned short port_no)
{
  unsigned long int  BaudRate;
  u8_t DataSize;
  u8_t Parity;
  u8_t StopSize;
  u8_t FlowControl;
  u8_t *Command = IACCommand;
  u8_t fcr_reg_val;
  unsigned int serial_index=0,t_flowcontrol=1;
  unsigned char t_parity=0,t_stop=0,t_data=0;
  
  /* Check wich command has been requested */
  switch (Command[3])
    {
      /* Signature */
    case TNCAS_SIGNATURE:
      if (IACPos == 6)
	{
	  /* Void signature, client is asking for our signature */
	  AddToBuffer (ToNetBuf, TNIAC);
	  AddToBuffer (ToNetBuf, TNSB);
	  AddToBuffer (ToNetBuf, TNCOM_PORT_OPTION);
	  AddToBuffer (ToNetBuf, TNASC_SIGNATURE);
	  SendStr (ToNetBuf, fw_version);
	  AddToBuffer (ToNetBuf, TNIAC);
	  AddToBuffer (ToNetBuf, TNSE);
	}
      else
	{
	  /* Received client signature */
	  strncpy (SigStr, (char *) &Command[4], IACPos - 6);
	}
      break;
      
      /* Set serial baud rate */
    case TNCAS_SET_BAUDRATE:
      /* Retrieve the baud rate which is in network order */
      BaudRate = ntohl(*((unsigned long int *) &Command[4]));
      
      
      if (BaudRate == 0)
	DPRINTK("Baudrate: Current\n");
      /* Client is asking for current baud rate */
      
      else
	{
	  /*
	  SetPortSpeed(PortFd,BaudRate);
	  */
	  serial_index = get_serial_index(port_no);
	  sp[serial_index].baudrate = BaudRate;
	  DPRINTK("si = %d port_no = %d\n",serial_index,port_no);
	  
	  //printk("address=%x,baudrate=%d,data=%d,stop=%d,parity=%d,flow=%d\n",&(serial99xx_ports[serial_index].port),sp[serial_index].baudrate,\
                        sp[serial_index].databits,sp[serial_index].stopbits, \
                        sp[serial_index].parity, sp[serial_index].flowcontrol);
		
#ifdef MCS9950_SS
	  ss_set_options(&(serial99xx_ports[serial_index].port),sp[serial_index].baudrate,\
		 	sp[serial_index].databits,sp[serial_index].stopbits, \
		 	sp[serial_index].parity, sp[serial_index].flowcontrol);
#else
	  mcs7840_set_serial_param(mcs8140_ss[serial_index], &sp[serial_index]);

#endif          
	  mcs8140_ss[serial_index]->baudrate=BaudRate;
          mcs8140_ss[serial_index]->serial_param=BAUD_RATE;
          Write_ConfigFile(mcs8140_ss[serial_index]);
	  if(BaudRate <= 4800) {
		mcs8140_ss[serial_index]->lower_baudrate=1;
	  }
	  else {
		mcs8140_ss[serial_index]->lower_baudrate=0;
	  }	  
	}	  
      
      /* Send confirmation */
      
      SendBaudRate (ToNetBuf,BaudRate);
      break;
      
      /* Set serial data size */
    case TNCAS_SET_DATASIZE:
      if (Command[4] == 0)
	DPRINTK("Data bits: Current\n");
      /*;Client is asking for current data size */
      else
	{
	  /* Set the data size */
	  //SetPortDataSize (PortFd, Command[4]);
	  t_data=Command[4];
	  serial_index = get_serial_index(port_no);

	  sp[serial_index].databits = t_data;//Command[4];
	   DPRINTK("si = %d port_no = %d\n",serial_index,port_no);
	  //printk("---------------------- 2 -------------------\n");
#ifdef MCS9950_SS
	  ss_set_options(&(serial99xx_ports[serial_index].port),sp[serial_index].baudrate,\
		 	sp[serial_index].databits,sp[serial_index].stopbits, \
		 	sp[serial_index].parity, sp[serial_index].flowcontrol);
#else
	  mcs7840_set_serial_param(mcs8140_ss[serial_index], &sp[serial_index]);

#endif          
	  mcs8140_ss[serial_index]->databits=t_data;//Command[4];
          mcs8140_ss[serial_index]->serial_param=DATA_BITS;
          Write_ConfigFile(mcs8140_ss[serial_index]);	  
	}
      /* Send confirmation */
      /* DataSize = GetPortDataSize (PortFd);*/
      DataSize = t_data;//Command[4]; //to be changed later..
      SendCPCByteCommand (ToNetBuf,TNASC_SET_DATASIZE, DataSize);
      break;
      
      
      /* Set the serial parity */
    case TNCAS_SET_PARITY:
      if (Command[4] == 0)
	DPRINTK("Parity: Current\n");
      /*; Client is asking for current parity */
      else
	{
	  /* Set the parity */
	  /*SetPortParity (PortFd, Command[4]);*/
	  t_parity=Command[4];
	  serial_index = get_serial_index(port_no);
	  sp[serial_index].parity   = t_parity;
          DPRINTK("si = %d port_no = %d\n",serial_index,port_no);
	  //printk("---------------------- 3 -------------------\n");
#ifdef MCS9950_SS
	  ss_set_options(&(serial99xx_ports[serial_index].port),sp[serial_index].baudrate,\
		 	sp[serial_index].databits,sp[serial_index].stopbits, \
		 	sp[serial_index].parity, sp[serial_index].flowcontrol);
#else
	  mcs7840_set_serial_param(mcs8140_ss[serial_index], &sp[serial_index]);
#endif
          mcs8140_ss[serial_index]->parity= t_parity;
          mcs8140_ss[serial_index]->serial_param=PARITY;
          Write_ConfigFile(mcs8140_ss[serial_index]);		  
	}
      
      /* Send confirmation */
      /* Parity = GetPortParity (PortFd);*/
      Parity = t_parity;//Command[4];//to be changed later..
      SendCPCByteCommand (ToNetBuf,TNASC_SET_PARITY, Parity);
      break;
      
      /* Set the serial stop size */
    case TNCAS_SET_STOPSIZE:
      if (Command[4] == 0)
	DPRINTK("Stop bits: Current\n");
      /*; Client is asking for current stop size */
      else
	{
	  /* Set the stop size */
	  /*SetPortStopSize (PortFd, Command[4]);*/
	  t_stop=Command[4];
	  serial_index = get_serial_index(port_no);
	  
          sp[serial_index].stopbits = t_stop;
          DPRINTK("si = %d port_no = %d\n",serial_index,port_no);
	  
	  //printk("---------------------- 4 -------------------\n");
#ifdef MCS9950_SS
	  ss_set_options(&(serial99xx_ports[serial_index].port),sp[serial_index].baudrate,\
		 	sp[serial_index].databits,sp[serial_index].stopbits, \
		 	sp[serial_index].parity, sp[serial_index].flowcontrol);
#else
	  mcs7840_set_serial_param(mcs8140_ss[serial_index], &sp[serial_index]);

#endif
          mcs8140_ss[serial_index]->stopbits=t_stop;
          mcs8140_ss[serial_index]->serial_param=STOP_BITS;
          Write_ConfigFile(mcs8140_ss[serial_index]);

	}
      
      /* Send confirmation */
      /* StopSize = GetPortStopSize (PortFd);*/
      StopSize = t_stop;//Command[4];// to be changed later..
      SendCPCByteCommand (ToNetBuf,TNASC_SET_STOPSIZE, StopSize);
      break;
      /* Flow control and DTR/RTS handling */
    case TNCAS_SET_CONTROL:
      switch (Command[4])
	{
	case 0:
	case 4:
	case 7:
	case 10:
	case 13:
	  DPRINTK("case 0,4,7.10,13 Flow control: Current\n");
	  /* Client is asking for current flow control or DTR/RTS status */
	  t_flowcontrol = (unsigned int)Command[4];
	  FlowControl = Command[4];//GetPortFlowControl (PortFd, Command[4]);
	  SendCPCByteCommand (ToNetBuf,TNASC_SET_CONTROL, FlowControl);
	  break;
	  
	case 5:
	  /* Break command */
	  // ??tcsendbreak(PortFd,1);
	  //tcsendbreak(PortFd,1);
	  t_flowcontrol = (unsigned int)Command[4];
          DPRINTK("case 5\n");
	  BreakSignaled = True;
	  SendCPCByteCommand (ToNetBuf,TNASC_SET_CONTROL, Command[4]);
	  break;
	  
	case 6:
	  t_flowcontrol = (unsigned int)Command[4];
          DPRINTK("case 6\n");
	  BreakSignaled = False;
	  SendCPCByteCommand (ToNetBuf,TNASC_SET_CONTROL, Command[4]);
	  break;
	  //#ifndef IRDA_RS485_EXT
	default:
	  t_flowcontrol = (unsigned int)Command[4];
          DPRINTK("case  %d\n1-NoFlow 2-Sw 3-Hw 8-DTR-On 9-DTR-Off 11-RTS-On 12-RTS-Off 15-Sw (inbound) 16-Hw(inbound)\n",Command[4]);
	  /*case 1:
	    case 2:
	    case 3:
	    case 8:
            case 9:
	    case 11: 
            case 12:
            case 14:
            case 15:
            case 16:*/
	  /* Set the flow control */
	  serial_index = get_serial_index(port_no);
	            
	  //printk("---------------------- set serial flow -------------------\n");
#ifdef MCS9950_SS
          ss_set_serial_flowcontrol(mcs8140_ss[serial_index],&(serial99xx_ports[serial_index]),Command[4]);
#else
          mcs7840_set_serial_flowcontrol(mcs8140_ss[serial_index], Command[4]);
#endif
	  if(t_flowcontrol==2 || t_flowcontrol==15 )
         	mcs8140_ss[serial_index]->flowcontrol=2;               
          else if(t_flowcontrol==3 || t_flowcontrol==16)
         	mcs8140_ss[serial_index]->flowcontrol=3;
	  else if(t_flowcontrol==1 || t_flowcontrol==14)
	  	mcs8140_ss[serial_index]->flowcontrol=1;

	  //mcs8140_ss[serial_index]->flowcontrol=Command[4];


          /*if( (mcs8140_ss[serial_index]->flowcontrol==2) || (mcs8140_ss[serial_index]->flowcontrol==3) 
							 || (mcs8140_ss[serial_index]->flowcontrol==1)) {*/
             mcs8140_ss[serial_index]->serial_param=FLOW_CONTROL;
             Write_ConfigFile(mcs8140_ss[serial_index]);
          //}

	  /* Flow control status confirmation */
	  if (CiscoIOSCompatible && Command[4] >= 13 && Command[4] <= 16)
	    /* INBOUND not supported separately.
	       Following the behavior of Cisco ISO 11.3
	    */
	    FlowControl = 0;
	  else
	    /* Return the actual port flow control settings */
	    FlowControl = 1;//GetPortFlowControl (PortFd, 0);
	  
	  FlowControl = Command[4];
	  FlowControl = t_flowcontrol;
	  /*End */
	  SendCPCByteCommand (ToNetBuf,TNASC_SET_CONTROL, FlowControl);
	  break;
	}
      break;
      
      /* Set the line state mask */
    case TNCAS_SET_LINESTATE_MASK:
      
      /* Only break notification supported */
      LineStateMask = Command[4] & (u8_t) 16;
      SendCPCByteCommand (ToNetBuf,TNASC_SET_LINESTATE_MASK, LineStateMask);
      break;
      
      /* Set the modem state mask */
    case TNCAS_SET_MODEMSTATE_MASK:
      ModemStateMask = Command[4];
      
      SendCPCByteCommand (ToNetBuf,TNASC_SET_MODEMSTATE_MASK, ModemStateMask);
      break;
      
      /* Port flush requested */
    case TNCAS_PURGE_DATA:
      switch (Command[4])
	{
	  /* Inbound flush */
	case 1:
	  fcr_reg_val = 0x0b;
	  break;
	  /* Outbound flush */
	case 2:
	  fcr_reg_val = 0x0d;
	  break;
	  /* Inbound/outbound flush */
	case 3:
	  fcr_reg_val = 0x0f;
	  break;
	default:
	  break;
	}
      
      SendCPCByteCommand (ToNetBuf,TNASC_PURGE_DATA, Command[4]);
      break;
      
      /* Suspend output to the client */
    case TNCAS_FLOWCONTROL_SUSPEND:
      InputFlow = False;
      break;
      
      /* Resume output to the client */
    case TNCAS_FLOWCONTROL_RESUME:
      InputFlow = True;
      break;
      
      /* Unknown request */
    default:
      break;
    }
}

 /*
 * Send the specific telnet option to SockFd using Command as command
 */

void
SendTelnetOption (BufferType * ToNetBuf,u8_t Command, char Option)
{
  u8_t IAC = TNIAC;
  
  AddToBuffer (ToNetBuf, IAC);
  AddToBuffer (ToNetBuf, Command);
  AddToBuffer (ToNetBuf, Option);
}

 /* 
 * Common telnet IAC commands handling 
 */

void
HandleIACCommand (BufferType * ToNetBuf,unsigned short port_no)
{
  
  u8_t *Command = IACCommand;
  /* Check which command */
  switch (Command[1])
    {
      /* Suboptions */
    case TNSB:
      if (!(tnstate_ptr[Command[2]].is_will || tnstate_ptr[Command[2]].is_do))
	break;
      switch (Command[2])
	{
	  /* RFC 2217 COM Port Control Protocol option */
	case TNCOM_PORT_OPTION:
	  HandleCPCCommand ( ToNetBuf,port_no);
	  break;
	default:
	  break;
	}
      break;
      /* Requests for options */
    case TNWILL:
      
      switch (Command[2])
	{
	  /* COM Port Control Option */
	case TNCOM_PORT_OPTION:
	  /*Telnet COM Port Control Enabled (WILL). */
	  TCPCEnabled = True;
	  if (!tnstate_ptr[Command[2]].sent_do)
	    {
	      SendTelnetOption (ToNetBuf,TNDO, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_do = 1;
	  break;
	  /* Telnet Binary mode */
	case TN_TRANSMIT_BINARY:
	  
	  /*Telnet Binary Transfer Enabled (WILL). */
	  if (!tnstate_ptr[Command[2]].sent_do)
	    {
	      SendTelnetOption (ToNetBuf,TNDO, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_do = 1;
	  break;
	  /* Echo request not handled */
	case TN_ECHO:
	  
	  if (!tnstate_ptr[Command[2]].sent_do)
	    SendTelnetOption (ToNetBuf,TNDO, Command[2]);
	  tnstate_ptr[Command[2]].is_do = 1;
	  break;
	  /* No go ahead needed */
	case TN_SUPPRESS_GO_AHEAD:
	  if (!tnstate_ptr[Command[2]].sent_do)
	    {
	      SendTelnetOption (ToNetBuf,TNDO, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_do = 1;
	  break;
	  /* Reject everything else */
	default:
	  SendTelnetOption (ToNetBuf,TNDONT, Command[2]);
	  tnstate_ptr[Command[2]].is_do = 0;
	  break;
	}
      tnstate_ptr[Command[2]].sent_do = 0;
      tnstate_ptr[Command[2]].sent_dont = 0;
      break;
      /* Confirmations for options */
    case TNDO:
      switch (Command[2])
	{
	  /* COM Port Control Option */
	case TNCOM_PORT_OPTION:
	  
	  TCPCEnabled = True;
	  if (!tnstate_ptr[Command[2]].sent_will)
	    SendTelnetOption (ToNetBuf,TNWILL, Command[2]);
	  tnstate_ptr[Command[2]].is_will = 1;
	  break;
	  /* Telnet Binary mode */
	case TN_TRANSMIT_BINARY:
	  if (!tnstate_ptr[Command[2]].sent_will)
	    {
	      SendTelnetOption (ToNetBuf,TNWILL, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_will = 1;
	  break;
	  /* Echo request handled.  The modem will echo for the user. */
	case TN_ECHO:
	  if (!tnstate_ptr[Command[2]].sent_will)
	    {
	      SendTelnetOption (ToNetBuf,TNWILL, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_will = 1;
	  break;
	  /* No go ahead needed */
	case TN_SUPPRESS_GO_AHEAD:
	  if (!tnstate_ptr[Command[2]].sent_will)
	    {
	      SendTelnetOption (ToNetBuf,TNWILL, Command[2]);
	    }
	  tnstate_ptr[Command[2]].is_will = 1;
	  break;
	  /* Reject everything else */
	default:
	  SendTelnetOption (ToNetBuf,TNWONT, Command[2]);
	  tnstate_ptr[Command[2]].is_will = 0;
	  break;
	}
      tnstate_ptr[Command[2]].sent_will = 0;
      tnstate_ptr[Command[2]].sent_wont = 0;
      break;
      /* Notifications of rejections for options */
    case TNDONT:
      if (tnstate_ptr[Command[2]].is_will)
	{
	  SendTelnetOption (ToNetBuf,TNWONT, Command[2]);
	  tnstate_ptr[Command[2]].is_will = 0;
	}
      tnstate_ptr[Command[2]].sent_will = 0;
      tnstate_ptr[Command[2]].sent_wont = 0;
      break;
    case TNWONT:
      if (Command[2] == TNCOM_PORT_OPTION)
	{
	  ;     /*Client doesn't support Telnet COM Port "
		  "Protocol Option (RFC 2217), trying to serve anyway. */
	}
      else
	{
	  ;       /*Received rejection for option */
	}
      if (tnstate_ptr[Command[2]].is_do)
	{
	  SendTelnetOption (ToNetBuf,TNDONT, Command[2]);
	  tnstate_ptr[Command[2]].is_do = 0;
	}
      tnstate_ptr[Command[2]].sent_do = 0;
      tnstate_ptr[Command[2]].sent_dont = 0;
      break;
    }
}

 /* 
 * Initialize a buffer for operation 
 */

void
InitTelnetStateMachine (struct _tnstate *tnstate_ptr1)
{
  u8_t i;
  
  for (i = 0; i < 63; i++)
    {
      tnstate_ptr1[i].sent_do = 0;
      tnstate_ptr1[i].sent_will = 0;
      tnstate_ptr1[i].sent_wont = 0;
      tnstate_ptr1[i].sent_dont = 0;
      tnstate_ptr1[i].is_do = 0;
      tnstate_ptr1[i].is_will = 0;
    }
}

 /* 
 * Initialize a buffer for operation
 */

void
InitBuffer (BufferType * B)
{
  /* Set the initial buffer positions */
  u8_t i = 0;
  
  for (; i < 128; i++)
    {
      B->Buffer[i] = '0';
    }
  
  B->RdPos = 0;
  B->WrPos = 0;
}

 /* 
 * Redirect char C to Device checking for IAC escape sequences 
 */

void
EscRedirectChar (u8_t C,BufferType * ToNetBuf,unsigned short port_no)
{
  static u8_t Last = 0;
  
  /* Check the IAC escape status */
  switch (IACEscape)
    {
      /* Normal status */
    case IACNormal:
      if (C == TNIAC)
	IACEscape = IACReceived;
      break;
      
      /* IAC previously received */
    case IACReceived:
      if (C == TNIAC)
	{
	  //AddToBuffer(&DevB,C);// ????
	  /* we should not have come here 8 ?? */
	  IACEscape = IACNormal;
	}
      else
	{
	  IACCommand[0] = TNIAC;
	  IACCommand[1] = C;
	  IACPos = 2;
	  IACEscape = IACComReceiving;
	  IACSigEscape = IACNormal;
	}
      break;
      
      /* IAC Command reception */
    case IACComReceiving:
      /* Telnet suboption, could be only CPC */
      if (IACCommand[1] == TNSB)
	{
	  /* Get the suboption signature */
	  if (IACPos < 4)
	    {
	      IACCommand[IACPos] = C;
	      IACPos++;
	    }
	  else
	    {
	      /* Check which suboption we are dealing with */
	      switch (IACCommand[3])
		{
		  /* Signature, which needs further escaping */
		case TNCAS_SIGNATURE:
		  
		  switch (IACSigEscape)
		    {
		    case IACNormal:
		      
		      if (C == TNIAC)
			IACSigEscape = IACReceived;
		      else if (IACPos < TmpStrLen)
			{
			  IACCommand[IACPos] = C;
			  IACPos++;
			}
		      break;
		      
		    case IACComReceiving:
		      
		      IACSigEscape = IACNormal;
		      break;
		      
		    case IACReceived:
		      
		      if (C == TNIAC)
			{
			  if (IACPos < TmpStrLen)
			    {
			      IACCommand[IACPos] = C;
			      IACPos++;
			    }
			  IACSigEscape = IACNormal;
			}
		      else
			{
			  if (IACPos < TmpStrLen)
			    {
			      IACCommand[IACPos] = TNIAC;
			      IACPos++;
			    }
			  
			  if (IACPos < TmpStrLen)
			    {
			      IACCommand[IACPos] = C;
			      IACPos++;
			    }
			  
			  HandleIACCommand (ToNetBuf,port_no);
			  IACEscape = IACNormal;
			}
		      break;
		    }
		  break;
		  
		  /* Set baudrate */
		case TNCAS_SET_BAUDRATE:
		  IACCommand[IACPos] = C;
		  IACPos++;
		  
		  if (IACPos == 10)
		    {
		      HandleIACCommand (ToNetBuf,port_no);
		      IACEscape = IACNormal;
		    }
		  break;
		  
		  /* Flow control command */
		case TNCAS_FLOWCONTROL_SUSPEND:
		case TNCAS_FLOWCONTROL_RESUME:
		  IACCommand[IACPos] = C;
		  IACPos++;
		  
		  if (IACPos == 6)
		    {
		      HandleIACCommand (ToNetBuf,port_no);
		      IACEscape = IACNormal;
		    }
		  break;
		  
		  /* Normal CPC command with single byte parameter */
		default:
		  IACCommand[IACPos] = C;
		  IACPos++;
		  
		  if (IACPos == 7)
		    {
		      HandleIACCommand (ToNetBuf,port_no);
		      IACEscape = IACNormal;
		    }
		  break;
		}
	    }
	}
      else
	{
	  /* Normal 3 byte IAC option */
	  IACCommand[IACPos] = C;
	  IACPos++;
	  
	  if (IACPos == 3)
	    {
	      HandleIACCommand (ToNetBuf,port_no);
	      IACEscape = IACNormal;
	    }
	}
      break;
    }
  /* Set last received byte */
  Last = C;
}
