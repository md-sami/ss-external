/*
 * ----------------
 * rfc2217cmdextr.c
 * ----------------
 * rfc2217cmdextr.c -- this file contains routines to 
 * parse the buffer received from the socket and saparate 
 * the data from the rfc2217 commands. The commands extracted
 * using these routines are parsed in rfc2217.c to generate
 * respective responses.
 *
 * Project Name : MCS8140-SerialServer-2.1
 * File Revision: 1.1
 * Author       : sami
 * Date Created : Thu Apr 3 2018
 *
 * Modification History:
 * 
 * $Id: rfc2217cmdextr.c,v 1.1 2010/06/28 14:00:42 sami Exp $
 *
 */
#include "rfc2217.h"
#include "data_types.h"
#include "debug.h"
#include "sockets.h"

spinlock_t		rfc_lock;

/*
 * extract_rfc2217_command -- extracts the rfc2217 command from the 
 * block and copies it in rfc2217_command[].
 * it returns the size of the command extracted.
 */

int  
extract_rfc2217_command(int *offset,unsigned char rfc2217_command[], unsigned char block[])
{

  int size=0;
  int position=1;
  int bufpos;
  
  rfc2217_command[0] = TNIAC; /* set first byte of command to 0xff */
  
  /*check the offset to find which command we received */
   switch(block[*offset])
    {
    case TNSB:    /* received com port configuration command*/
                  /* check which command we received */
      
      /*if it is 7 byte command */        
      if( (block[*offset+5] == TNSE) && (block[*offset+4]) == TNIAC)
	{
	  for(bufpos=0;bufpos<6;bufpos++)
	    {
	      rfc2217_command[position]=block[*offset+bufpos];
	      position++;
	    }
	  size=position;
	  *offset+=5;
          return(size);
        }  
      
      /*if it is 10 byte command */ 
      else if( (block[*offset+8]== TNSE) && (block[*offset+7] == TNIAC) )
        {
          for(bufpos=0;bufpos<9;bufpos++)
            {
              rfc2217_command[position]=block[*offset+bufpos];
              position++;
            }
          size=position;
          *offset+=8;
          return(size);
        }
      
      /*if it is 6 byte command */
      else if( (block[*offset+4]== TNSE) && (block[*offset+3] == TNIAC) )
        {
          for(bufpos=0;bufpos<5;bufpos++)
	    {
              rfc2217_command[position]=block[*offset+bufpos];
              position++;
            }
          size=position;
          *offset+=4;
          return(size);
        }
      break;
      
    case TNWILL:    /* received negotiation command:FF FB 2C */
      rfc2217_command[position]=block[*offset];
      
      if(  (block[*offset+1]) == TNCOM_PORT_OPTION)
        {
          rfc2217_command[position+1]=block[*offset+1];
          *offset+=1;
        }
      size=3;
      return (size);
      break;
      
    case TNWONT:   /*received negotiation command: FF FC 2C */
      rfc2217_command[position]=block[*offset];
      if( (block[*offset+1]) == TNCOM_PORT_OPTION)
        {
          rfc2217_command[position+1]=block[*offset+1];
          *offset+=1;
          size=3;
          return(size);
        }
      break;
    
    default:
	printk("invalid command received\n");
        break;
    
   }


    return(size);
}

/*
 * send_rfc2217_cmd_response -- gets the response for the 
 * rfc2217_command  and sends it to the client.
 */
int 
send_rfc2217_cmd_response(unsigned char rfc2217_command[],int size,struct socket *sockfd,unsigned short port_no)
{

  int index;
  unsigned long flags;

#ifdef MCS7840_SS
  spin_lock_irqsave(&rfc_lock,flags);
#else
  spin_lock(&rfc_lock);
#endif
  
IACEscape = IACNormal;
  IACSigEscape = IACNormal;
  BufferType ToNetBuf;

  ToNetBuf.WrPos=0;
  
  memset(ToNetBuf.Buffer,0,sizeof(ToNetBuf.Buffer));
 
  for(index=0;index<size;index++)
    {
      EscRedirectChar(rfc2217_command[index],&ToNetBuf,port_no);
    }

#ifdef MCS7840_SS
  spin_unlock_irqrestore(&rfc_lock,flags); 
  send_buffer(sockfd,ToNetBuf.Buffer,ToNetBuf.WrPos);
#else
  send_buffer(sockfd,ToNetBuf.Buffer,ToNetBuf.WrPos);
  spin_unlock(&rfc_lock);
#endif

 	/*for(index=0;index < ToNetBuf.WrPos;index++) {
	printk("%d ",ToNetBuf.Buffer[index]);
  }
  printk("\n");
  */

  
  return(True);

}

/*
 * bfstrchr -- search for char ch in buffer.
 */
int bfstrchr(unsigned char *buff,unsigned char ch,int n)
{
   int index;

   if(buff == NULL) return (-1);

   for(index=0;index<n;index++)
      {
       if(buff[index]==ch) 
         {
          return index;
         }
       index++;
      }
   return -1;
}

/*
 * char_to_int -- convert a character to integer and return.
 */
int char_to_int(char *str)
{
       int val,i;
       val =0;
       for(i=0;str[i]!='\0';i++)
               val = (val*10) + (str[i] - 48);
       return val;
}






