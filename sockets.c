/*
 * ---------
 * sockets.c
 * ---------
 * sockets.c file has the functions for TCP sockets.
 *
 * Project Name : MCS8140-SerialServer-2.1
 * File Revision: 1.1
 * Author       : sami
 * Date Created : Thu Apr 3 2018
 *
 * Modification History:
 *
 * $Id: sockets.c,v 1.1 2018/06/28 11:46:20 sami Exp $
 *
 */

#include "sockets.h"
#include "config.h"

/*
 * socket_create -- creates the TCP socket on port ServerPort
 * returns 0 on success and negetive value on error.
 */

struct socket* Socket;

struct socket* Get_Socket;

int 
socket_create(void)
{
  struct sockaddr_in server;
  int error;
  char *destino;
  int ServerPort=USER_MODE_SERVER_PORT;
  
  error = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&Socket);
  if (error<0)
    PRINT_ERR("Error during creation of socket  terminating Socket Create\n");
  else
    PRINT_INFO("Socket  OK\n");
  
  destino = USER_MODE_SERVER_IP;
  
  server.sin_family       = AF_INET;
  server.sin_addr.s_addr  = in_aton(destino);
  server.sin_port         = htons(ServerPort);
  Socket->sk->sk_reuse = 1;
  
  error = Socket->ops->connect(Socket,(struct sockaddr*)&server,sizeof(server),0);
  
  if (error<0)
    PRINT_ERR("Socket_Create ERROR connect error = %d Socket \n",error);
  
  return 0;
}
int 
socket_create_get(void)
{
  struct sockaddr_in get_server;
  int error;
  char *destino;
  int ServerPort=USER_MODE_SERVER_GET_PORT;
  
  error = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&Get_Socket);
  if (error<0)
    PRINT_ERR("Error during creation of socket  terminating Get_Socket Create\n");
  else
    PRINT_INFO("Get_Socket  OK\n");
  
  destino = USER_MODE_SERVER_IP;
  
  get_server.sin_family       = AF_INET;
  get_server.sin_addr.s_addr  = in_aton(destino);
  get_server.sin_port         = htons(ServerPort);
  Get_Socket->sk->sk_reuse = 1;
  
  error = Get_Socket->ops->connect(Get_Socket,(struct sockaddr*)&get_server,sizeof(get_server),0);
  
  if (error<0)
    PRINT_ERR("Socket_Create ERROR connect error = %d Get_Socket \n",error);
  
  return 0;
}


/*
 * send_buffer --  sends buffer of length len_in to the socket.
 * using sock_sendmsg.
 * on success returns number of bytes written to the socket.
 * on failure it returns error
 */
int  
send_buffer (struct socket *sock, const char *buf, const size_t len_in)
{
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov;
  int len;
  
  if (!sock->sk)
    {
      PRINT_ERR ("send_buffer: invalid socket\n");
      return -ECONNRESET;
    }
  
  msg.msg_name = 0;                       /* Socket name                  */
  msg.msg_namelen = 0;                    /* Length of name               */
  
  msg.msg_iov = &iov;                     /* Data blocks                  */
  msg.msg_iovlen = 1;                     /* Number of blocks             */
  
  msg.msg_control = NULL;                 /* Per protocol magic (eg BSD file descriptor passing) */
  msg.msg_controllen = 0;                 /* Length of cmsg list */
  
  msg.msg_flags = 0;                      //MSG_DONTWAIT | MSG_NOSIGNAL;
  
  msg.msg_iov->iov_base = (char *) buf;
  
  msg.msg_iov->iov_len = (__kernel_size_t) len_in;
  
  oldfs = get_fs ();
  set_fs (KERNEL_DS);
  len = sock_sendmsg (sock, &msg, (size_t) (len_in));
  set_fs (oldfs);
  
  return len;
}

/*
 * recv_buffer --  reads buffer of length size from the socket.
 * using sock_recvmsg.
 * on success returns number of bytes read from the  socket.
 * on failure it returns error
 */
int
recv_buffer (struct socket *sock, char *buf, size_t size)
{
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov;
  int len;
  
  if (!sock->sk)
    {
      PRINT_ERR ("recv_buffer: invalid socket\n");
      return -ECONNRESET;
    }
  
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  
  msg.msg_flags = 0;
  
  msg.msg_iov->iov_base = (char *) buf;
  msg.msg_iov->iov_len = (__kernel_size_t) size;
  
  oldfs = get_fs ();
  set_fs (KERNEL_DS);
  
  len = sock_recvmsg (sock, &msg, size, 0);
  
  set_fs (oldfs);
  
  return len;
}

/*
 * Udp_send_buffer --  sends buffer of length len_in to the socket.
 * using sock_sendmsg.
 * on success returns number of bytes written to the socket.
 * on failure it returns error
 */
int
Udp_send_buffer (struct socket *sock, unsigned char *buf,int len_in,nw2serial_t * wap)
{
  mm_segment_t oldfs;
  struct iovec iov;
  struct msghdr msg;
  int len;
  
  if (!sock->sk)
    {
      PRINT_ERR ("send_buffer: invalid socket\n");
      return -ECONNRESET;
    }
  
  msg.msg_name = (void *) &wap->server_details[0];              /* Socket name                  */
  msg.msg_namelen = sizeof(struct sockaddr_in);                 /* Length of name               */
  
  msg.msg_iovlen = 1;                                           /* Number of blocks             */
  msg.msg_control = NULL;         /* Per protocol magic (eg BSD file descriptor passing) */
  
  msg.msg_controllen = 0;         /* Length of cmsg list */
  msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;
  
  msg.msg_iov = &iov;             /* Data blocks                  */
  msg.msg_iov->iov_base = (char *) buf;
  msg.msg_iov->iov_len = (__kernel_size_t) len_in;
  
  oldfs = get_fs ();
  set_fs (KERNEL_DS);
  len = sock_sendmsg (sock, &msg, (size_t) (len_in));
  set_fs (oldfs);
  
  return len;
}

void
Udp_Send(nw2serial_t *wap,unsigned char *data, int len)
{
        int count =0;
        int count1 =0;
        int Last3_ip1 =0; /*For storing the last three digits of the StartingIp address*/
        int Last3_ip2 =0;/*For storing the last three digits of the Ending Ip address*/
        char Last_Three[3];
        int difference = 0;
        char receiver_ip[16] ;
        //int S_Port = 1234;
        struct msghdr msg;

        memset(&msg, 0, sizeof(struct msghdr));

        difference=Last3_ip2-Last3_ip1;
        for(count =0;count<wap->No_Udp_receivers;count++)
        {
                Get_Receiver_Ip_From_Conifg_file(wap->Receiver_Start_ip[count],Last_Three,0);
                Last3_ip1= atoi(Last_Three);
                strcpy(receiver_ip,wap->Receiver_Start_ip[count]);
                Get_Receiver_Ip_From_Conifg_file(wap->Receiver_End_ip[count],Last_Three,0);
                Last3_ip2=atoi(Last_Three);

                difference=Last3_ip2-Last3_ip1;
                //printk("Difference.....%d\n",difference);

                for(count1=0;count1<=difference;count1++)
                {
                        Last3_ip1++;
                        //printk("%s\n",receiver_ip);
                        wap->server_details[0].sin_family = AF_INET;
                        wap->server_details[0].sin_addr.s_addr = in_aton(receiver_ip);
                        wap->server_details[0].sin_port = htons(wap->Destination_Port[count]);
                //      printk("UdpPort=%d\n",wap->Destination_Port[count]);
                        Get_Receiver_Ip_From_Conifg_file(receiver_ip,NULL,Last3_ip1);
                        Udp_send_buffer(wap->server,data,len, wap) ;
                }
        }
}

/*
 * check the state of socket, this is called before closing socket to 
 * check its state.
 * on success it returns 1 and on failure it returns 0
 */
int checking_state_of_socket(nw2serial_t * wap,int Client_Index)
{
  if(wap->client->sk->sk_state==TCP_CLOSE || wap->client->sk->sk_state==TCP_CLOSE_WAIT)
    {
      PRINT_INFO("TCP Connection on wap->port : %d Closed\n",wap->port);
      sock_release(wap->client);
      wap->client = NULL;
      return 1;
    }
  return 0;
}

/*
 * socket_status -- returns the current state
 * of the socket.
 */
void socket_status(nw2serial_t * wap,int client_Index,char *destination)
{
  switch(wap->client->sk->sk_state)
    {
    case 1:
      strcpy(destination,"TCP_ESTABLISHED");
      break;
    case 2:
      strcpy(destination,"TCP_SYN_SENT");
      break;
    case 3:
      strcpy(destination,"TCP_SYN_RECV");
      break;
    case 4:
      strcpy(destination,"TCP_FIN_WAIT1");
      break;
    case 5:
      strcpy(destination,"TCP_FIN_WAIT2");
      break;
    case 6:
      strcpy(destination,"TCP_TIME_WAIT");
      break;
    case 7:
      strcpy(destination,"TCP_CLOSE");
      break;
    case 8:
      strcpy(destination,"TCP_CLOSE_WAIT");
      break;
    case 9:
      strcpy(destination,"TCP_LAST_ACK");
      break;
    case 10:
      strcpy(destination,"TCP_LISTEN");
      break;
    case 11:
      strcpy(destination,"TCP_CLOSING");
      break;
    }
  return;
}

/*
 * This function will be called when ever Inactivity Time out occurs
 * This Function Releases the Socket and closes the connection
 */
void Release_Socket(unsigned long arg)
{
     	nw2serial_t *wap;
	wap = (nw2serial_t *) arg;
    	
	DPRINTK("timed out\n");
     	wap->InActivity_Enabled = 1;
	wakeup(wap);
}

void wakeup(nw2serial_t *wap)
{
	int index;

	/*if(wap->client != NULL) {
		if(wap->client->sk->sk_state  ==  TCP_ESTABLISHED) {
			wake_up(wap->client->sk->sk_sleep);
		}
	}*/

	if(wap->mode == server_mode) {
		for(index=0;index<4;index++){
			if(wap->tsrclient[index] != NULL) {
				if(wap->tsrclient[index]->sk != NULL) {
					wake_up(wap->tsrclient[index]->sk->sk_sleep);
				}
			}
		}
	}
	else {
		for(index=0;index<4;index++){
			if(wap->tcpclient[index] != NULL) {
				if(wap->tcpclient[index]->sk->sk_state  ==  TCP_ESTABLISHED) {
					wake_up(wap->tcpclient[index]->sk->sk_sleep);
				}
			}
		}
	}
}

/*
 * init_wap_client -                                         
 * This function is used to initialize the client socket
 */
void init_wap_client(nw2serial_t *wap)
{
        int index =0;
        for(index=0;index<NO_OF_SERVERS;index++)
        {
                wap->tsrclient[index]=NULL;
        }
        return;
}

/*
 * check_Which_client_socket_free -                             
 * This function is used to check which client socket is free 
 */
int check_which_client_socket_free(nw2serial_t * wap)
{
        int i =0;
        if(wap->mode == server_mode) {
                for(i=0;i<wap->Maximum_Connection;i++) {
                        if(wap->tsrclient[i]==NULL)
                        	return i;
                        	//continue;
                }
        }
        else {
                for(i=0;i<NO_OF_SERVERS;i++) {
                        if(wap->tcpclient[i]==NULL)
                                return i;
                        	//continue;
                }
        }
        return -1;
}

void release_tsrclients(nw2serial_t *wap)
{
	int i=0;
        if(wap->mode == server_mode) {
                for(i=0;i<wap->Maximum_Connection;i++) {
                        if(wap->tsrclient[i]==NULL) {
                        	sock_release(wap->tsrclient[i]);
                        	wap->tsrclient[i] = NULL;
			}
                }
        }
}


int 
ss_get_port_number(int port_index)
 {
  switch(port_index)
   {
     case 0	: return(1234);   
     case 1 	: return(1235); 
     case 2 	: return(1236); 
     case 3 	: return(1237); 

     case 4 	: return(1244); 
     case 5 	: return(1245); 
     case 6 	: return(1246); 
     case 7 	: return(1247); 

     case 8 	: return(1254); 
     case 9 	: return(1255); 
     case 10 	: return(1256); 
     case 11 	: return(1257); 

     case 12 	: return(1264); 
     case 13 	: return(1265); 
     case 14 	: return(1266); 
     case 15 	: return(1267); 

     case 16 	: return(1274); 
     case 17 	: return(1275); 
     case 18 	: return(1276); 
     case 19 	: return(1277); 

     case 20 	: return(1284); 
     case 21 	: return(1285); 
     case 22 	: return(1286); 
     case 23 	: return(1287); 

     case 24 	: return(1294); 
     case 25 	: return(1295); 
     case 26 	: return(1296); 
     case 27 	: return(1297); 

     case 28 	: return(1334); 
     case 29	: return(1335); 
     case 30 	: return(1336); 
     case 31 	: return(1337); 

     default    : return 0;
   }
 }
































 
