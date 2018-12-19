/*
 * --------------
 * config.c
 * --------------
 *
 * Project Name : MCS8140-SerialServer-2.1
 * File Revision: 1.1
 * Author       : sami
 * Date Created : Thu Apr 3 2018
 *
 * Modification History:
 *         Id  Date                By      Description
 */


#include "config.h"
#include "sockets.h"

/*
 * Read_ConfigFile -- this function Read the configfile in userspace.
 */

void Read_ConfigFile(nw2serial_t * wap)
{
  	int Index_Value = 0,No_Servers_Client_To_Connect  = 0;
  	int Config_index_Value = UDP_RECEIVER_IP_BASE + (SIZE_OF_OPR_WA * (wap->serial_index));
 	int Port_index = LOCAL_LISTEN_PORT_INDEX+ (SIZE_OF_OPR_WA * (wap->serial_index));
  	int Destination_Port_Index =IP_BASE_ADDRESS+1 + (SIZE_OF_OPR_WA * (wap->serial_index));
  	int Destination_Ip_Count =0,Server_Index =0;
  
  	switch(wap->mode) {
    		
		case real_comm_mode:
                        Index_Value = INACTIVITY_TIME+ (SIZE_OF_OPR_WA * (wap->serial_index));
                        Get_Configuration_Details(wap, Index_Value, INACTIVITY_TIME_FIELD,0);
                        PRINT_INFO("Inactivity_Time=%d\n",wap->InActivity_Time);
                        if(wap->InActivity_Time <1000 && wap->InActivity_Time != 0)
                  		{
                          	wap->InActivity_Time = 1;
                  	}
                  	else {
                   		wap->InActivity_Time = wap->InActivity_Time/1000;
                	}
  	
			Index_Value = KEEPALIVE_TIME + (SIZE_OF_OPR_WA * (wap->serial_index));
                  	Get_Configuration_Details(wap, Index_Value, KEEP_ALIVE_FEILD,0);
                 	
			break;
   		
		case server_mode:
			DPRINTK("in case server mode\n");
                        Index_Value = LOCAL_TCP_PORT_INDEX + (SIZE_OF_OPR_WA * (wap->serial_index));
                        Get_Configuration_Details(wap, Index_Value, PORT_FEILD,0);
                        PRINT_INFO("Local Port=%d\n",wap->Local_Listen_Port);

                        Index_Value = MAX_CONNECTION + (SIZE_OF_OPR_WA * (wap->serial_index));
                        Get_Configuration_Details(wap, Index_Value, MAX_CONNECTION_FEILD,0);
                        Index_Value = INACTIVITY_TIME+ (SIZE_OF_OPR_WA * (wap->serial_index));
                        Get_Configuration_Details(wap, Index_Value, INACTIVITY_TIME_FIELD,0);
                        PRINT_INFO("Inactivity_Time=%d\n",wap->InActivity_Time);

                  	if(wap->InActivity_Time <1000 && wap->InActivity_Time != 0) {
                          	wap->InActivity_Time = 1;
                  	}
                  	else {
                   	wap->InActivity_Time = wap->InActivity_Time/1000;
			}
		
                  	Index_Value = KEEPALIVE_TIME + (SIZE_OF_OPR_WA * (wap->serial_index));
                  	Get_Configuration_Details(wap, Index_Value, KEEP_ALIVE_FEILD,0);
                        break;
        
   		case client_mode:
    			DPRINTK("in case client mode\n");
    			Index_Value = IP_BASE_ADDRESS+ (SIZE_OF_OPR_WA * (wap->serial_index));
   
    			for(Server_Index =0 ;Server_Index < NO_OF_SERVERS;Server_Index++) {
				if(Get_Configuration_Details(wap, Index_Value, SERVER_IP_FEILD,Server_Index) == 0) {
	    				Server_Index = NO_OF_SERVERS;
	    				continue;
	  			}
	
				Index_Value = Index_Value + 1;
	
				if(Get_Configuration_Details(wap, Index_Value, SERVER_PORT_FEILD,Server_Index) == 0) {
	    				Server_Index = NO_OF_SERVERS;
	    				continue;
	  			}
				else
	  			No_Servers_Client_To_Connect= No_Servers_Client_To_Connect +1;
	
				PRINT_INFO("No_Servers_Client_To_Connect....%d\n",No_Servers_Client_To_Connect);
				Index_Value = Index_Value + 1;
      			}
    
    			wap->No_Tcp_Clients = No_Servers_Client_To_Connect;
    			Index_Value = TCP_CONNECT_ON_INDEX + (SIZE_OF_OPR_WA * (wap->serial_index));
    			Get_Configuration_Details(wap, Index_Value, TCP_CONNECT_ON_FEILD,0);
    			Index_Value = KEEPALIVE_TIME + (SIZE_OF_OPR_WA * (wap->serial_index));
    			Get_Configuration_Details(wap,Index_Value, KEEP_ALIVE_FEILD,0);
    			Index_Value = INACTIVITY_TIME +(SIZE_OF_OPR_WA * (wap->serial_index));
    			Get_Configuration_Details(wap, Index_Value, INACTIVITY_TIME_FIELD,0);
    
			if(wap->InActivity_Time <1000 && wap->InActivity_Time != 0) {
				wap->InActivity_Time = 1;
      			}
    			else {
      				wap->InActivity_Time = wap->InActivity_Time/1000;
    			}

			PRINT_INFO("Inactivity_Time=%d\n",wap->InActivity_Time);
    			break;
        
		 case udp_mode:
                	wap->No_Udp_receivers =0;
			printk("--- Port_Index = %d\n",Port_index);
                	Get_Configuration_Details(wap, Port_index, PORT_FEILD, 0);
                	printk("Local_Listen_Port = %d\n",wap->Local_Listen_Port);

                	for(Destination_Ip_Count =0;Destination_Ip_Count<4;Destination_Ip_Count++) {
                        	if(Get_Configuration_Details(wap,Config_index_Value , RECEIVER_START_IP_FEILD,Destination_Ip_Count)==0) {
                                	Destination_Ip_Count = 4;
                                	continue;
                        	}
                        	printk("Receiver_Start_ip %s\n",wap->Receiver_Start_ip[Destination_Ip_Count]);

                        	Config_index_Value++;

                        	Get_Configuration_Details(wap,Config_index_Value , RECEIVER_END_IP_FEILD,Destination_Ip_Count);
                        	printk("Receiver_End_ip %s\n",wap->Receiver_End_ip[Destination_Ip_Count]);

                        	Get_Configuration_Details(wap,Destination_Port_Index , DESTINATION_PORT_FEILD,Destination_Ip_Count);
                        	printk("Destionatin_Port= %d\n",wap->Destination_Port[Destination_Ip_Count]);

                        	Destination_Port_Index=Destination_Port_Index+2;

                        	Config_index_Value++;
                        	wap->No_Udp_receivers++;
                	}

                	break;
  	} 
} 

/*
 * Write_ConfigFile -- this function Read the configfile in userspace.
 */

void Write_ConfigFile(nw2serial_t * wap)
{
  int Index_Value=1;
  char S_buff[60];
  int port_index;

  memset(S_buff,0x0,sizeof(S_buff));
  
  //printk("in Write_Configfile function\n");
  //printk("before get_port_index port is %d\n",wap->port); 
  
  port_index=get_port_index(wap->port);
 
  switch(wap->serial_param) {
   case BAUD_RATE:
    Index_Value=port_index;
    DPRINTK("wap->baudrate=%ld\n",wap->baudrate);
    sprintf(S_buff,"%d;%ld;put",Index_Value,wap->baudrate); 
    break;

   case DATA_BITS:
    Index_Value=port_index+1;
    DPRINTK("wap->databits=%d\n",wap->databits);
    sprintf(S_buff,"%d;%d;put",Index_Value,wap->databits); 
    break;

   case STOP_BITS:
    Index_Value=port_index+2;
    DPRINTK("wap->stopbits=%d\n",wap->stopbits);
    sprintf(S_buff,"%d;%d;put",Index_Value,wap->stopbits); 
    break;
   
   case PARITY:
    Index_Value=port_index+3;
    DPRINTK("wap->parity=%d\n",wap->parity);
    sprintf(S_buff,"%d;%d;put",Index_Value,wap->parity); 
    break;

   case FLOW_CONTROL:
    Index_Value=port_index+4;
    DPRINTK("wap->flowcontrol=%d for port %d\n",wap->flowcontrol,wap->port);
    sprintf(S_buff,"%d;%d;put",Index_Value,wap->flowcontrol); 
    break;
  }
 
  send_buffer(Socket, S_buff, sizeof(S_buff));

} 
/*
 * Get_Configuration_Details -- called from Read_ConfigFile, this function
 * sends the Index_Value to server running in user space ang reads the value in 
 * R_buff.
 */

int  Get_Configuration_Details(nw2serial_t * wap,int Index_Value, int Which_Field,int Socket_Index)
{
 	char S_buff[60];
  	char R_buff[60];
  
	memset(S_buff,0x0,sizeof(S_buff));
  	memset(R_buff,0x0,sizeof(R_buff));
  
  	sprintf(S_buff,"%d;get",Index_Value);
  	send_buffer(Get_Socket, S_buff, strlen(S_buff));
  
  	recv_buffer(Get_Socket, R_buff, sizeof(R_buff));
  	//DPRINTK("received from configserver:%s\n",R_buff);

        if((strcmp(R_buff,"NULL")==0)){
                printk("received No_Value from configfile for %d\n",Index_Value);
                return 0;
        }
 
  	switch(Which_Field) {
    		
		case INACTIVITY_TIME_FIELD:
      			wap->InActivity_Time= atoi(R_buff);
			DPRINTK("inactivity time=%d\n",wap->InActivity_Time);
      			break;
	  
    		case KEEP_ALIVE_FEILD:
           		wap->Keep_Alive_Time = atoi(R_buff);
           		DPRINTK("keepalive time%d\n",wap->Keep_Alive_Time);
           		break;
      
      		case TCP_CONNECT_ON_FEILD:
           		if(!(strcmp(R_buff,"anycharacter"))){
             			wap->Tcp_Connect_On_State = 1;
             			wap->startup_mode = on_char;
             			DPRINTK("tcp_connect_on...state=1\n");
           		}
           		else {
             			wap->Tcp_Connect_On_State =  0;
             			wap->startup_mode = on_start;
             			DPRINTK("tcp_connect_on_state=%d\n",wap->Tcp_Connect_On_State);
           		} 
           		break;
    		
		case MAX_CONNECTION_FEILD:
     			wap->Maximum_Connection = atoi(R_buff);
     			break;
 
      		case SERVER_IP_FEILD:
                        if((strcmp(R_buff,"NULL")==0)){
                                return 0;
                        }

           		DPRINTK("Server_IP_Field %s\n",R_buff);
           		wap->server_details[Socket_Index].sin_family = AF_INET;
           		wap->server_details[Socket_Index].sin_addr.s_addr = in_aton(R_buff);
           		break;
    
      		case SERVER_PORT_FEILD:
                        if((strcmp(R_buff,"NULL")==0)){
                                return 0;
                        }

           		wap->server_details[Socket_Index].sin_port = htons(atoi(R_buff));
           		DPRINTK("Server_Port%d\n",atoi(R_buff));
           		break;
                
		case PORT_FEILD:
                        if((strcmp(R_buff,"NULL")==0)){
                                return 0;
                        }
                        wap->Local_Listen_Port = atoi(R_buff);
                        break;

                case RECEIVER_START_IP_FEILD:
                        if((strcmp(R_buff,"NULL")==0)){
                                return 0;
                        }
                        /*Note Socket Index is Destion Ip Count*/
                        memset(wap->Receiver_Start_ip[Socket_Index],0x0,sizeof(wap->Receiver_Start_ip[Socket_Index]));
                        strcpy(wap->Receiver_Start_ip[Socket_Index],R_buff);
                        break;
                        /*Receiver End IP*/
                
		case RECEIVER_END_IP_FEILD:
                        /*Note Socket Index is Destion Ip Count*/
                        memset(wap->Receiver_End_ip[Socket_Index],0x0,sizeof(wap->Receiver_End_ip[Socket_Index]));
                        strcpy(wap->Receiver_End_ip[Socket_Index],R_buff);
                        break;

                case DESTINATION_PORT_FEILD :
                        wap->Destination_Port[Socket_Index] = atoi(R_buff);
                        break;

      
 	      /*case LOCAL_IP:
			strcpy(wap->local_ip,R_buff);
			break;*/
  	}
  
  return 1;
}

int get_config_string(int Index_Value,char *R_buff)
{
  char S_buff[60];
  
  memset(S_buff,0x0,sizeof(S_buff));
  sprintf(S_buff,"%d;get",Index_Value);
  send_buffer(Get_Socket, S_buff,strlen(S_buff));
  recv_buffer(Get_Socket, R_buff,sizeof(S_buff));
  return 1;
}

/*
 * atoi -- This function converts the ascii value to interger value 
 */
//inline int atoi(char s [])
int atoi(char s [])
{
  int i = 0;
  int n =0;
  for(i=0;s[i]>='0' && s[i]<='9';i++) {
      n=n*10 + (s[i] -  '0');
  }
  
  return n;
}

/*
 * get_port_index -- gets configfile port index
 */

int get_port_index(int portno)
{

 switch(portno) {
     
    case 1234 : return 108;
    case 1235 : return 117;
    case 1236 : return 126; 
    case 1237 : return 135;

    case 1244 : return 144;
    case 1245 : return 153;
    case 1246 : return 162;
    case 1247 : return 171;

    case 1254 : return 180;
    case 1255 : return 189;
    case 1256 : return 198;
    case 1257 : return 207;

    case 1264 : return 216; 
    case 1265 : return 225;
    case 1266 : return 234;
    case 1267 : return 243;

    case 1274 : return 252;
    case 1275 : return 261;
    case 1276 : return 270;
    case 1277 : return 279;

    case 1284 : return 288;
    case 1285 : return 297;
    case 1286 : return 306;
    case 1287 : return 315;

    case 1294 : return 324;
    case 1295 : return 333;
    case 1296 : return 342;
    case 1297 : return 351;

    case 1334 : return 360;
    case 1335 : return 369;
    case 1336 : return 378;
    case 1337 : return 387;

    default   : return 1;
 }
 return 1;
}


/*
 * This Function is used to calculate the difference
 * between two different Numbers
 */
int Get_Receiver_Ip_From_Conifg_file(char *ip ,char * lasthree,int len)
{
        int count =0;
        char Last_Three[3];
        int i =0;
        while(*ip!='\0')
        {
                if(*ip=='.')
                        ++count;
                if(count==3)
                {
                        ip++;
                        break;
                }
                ip++;
        }
        if(len ==0)
        {
                while(*ip!='\0')
                {
                        *lasthree++=*ip++;
                }
        }
        else
        {
                sprintf(Last_Three,"%d",len);
                while(Last_Three[i] !='\0')
                {
                        *ip++ = Last_Three[i];
                        i++;
                }
        }
        return 0;
}
 
