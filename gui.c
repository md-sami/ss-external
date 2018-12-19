
#include "gui.h"
#include "config.h"
extern int xit;
int num_gui_threads=0;

int if_task_gui(void *arg)
{
        struct gui_thrd *gui = (struct gui_thrd *)arg;
        struct sockaddr_in server;
        int Error_Accept =0,nread,nsend;
        unsigned char *buffer;
        unsigned char *recv_buff;
        

	lock_kernel();
        daemonize("if_task_gui");
        allow_signal(SIGKILL);
        unlock_kernel();

        //set_user_nice(current,-5); /* GUI thrds have more priority than any other (earlier it was -10)*/
        complete(&gui->gui_complete);

        sprintf(current->comm,"if_task_gui%d",gui->portno);
        buffer= kzalloc(60,GFP_ATOMIC);
        if(!buffer) {
                printk("failed to allocate buffer in if_task_gui\n");
        }

        recv_buff = kzalloc(2048,GFP_ATOMIC);
        if(!recv_buff) {
                printk("failed to allocate recv_buff in if_tak_gui\n");
        }

Accept:
        memset(recv_buff,0,2048);
        memset(buffer,0,60);
       
	if(xit)
                goto exit;

        if (0 > sock_create (PF_INET, SOCK_STREAM, IPPROTO_TCP, &gui->server)) {
                PRINT_ERR ("Error during creation of socket; terminating\n");
                goto exit;
        }
	else DPRINTK("sock create success\n");

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons (gui->portno);
        gui->server->sk->sk_reuse = 1;

        if (0 > gui->server->ops->bind (gui->server, (struct sockaddr *) &server, sizeof (server))) {
                PRINT_ERR (KERN_ALERT "!!!!! ERROR BINDING SOCKET %d !!!!!!!\n", gui->portno);
                goto exit;
        }
	else DPRINTK("bind success\n");

        if (gui->server->ops->listen (gui->server,1 )) {
                PRINT_ERR(KERN_ALERT " Error listening on socket\n");
                goto exit;
        }
        else DPRINTK("listen success %s , %d\n",__FUNCTION__,__LINE__);

        if(sock_create_lite(PF_INET,gui->server->type,IPPROTO_TCP,&gui->client)) {
                DPRINTK("error Creating new socket gui->client\n");
        }
	else DPRINTK("sock_create lite  success\n");

        if(gui->client == NULL){
                PRINT_ERR(KERN_ALERT " error during creation gui->client\n");
        }

        gui->client->type = gui->server->type;
        gui->client->ops =  gui->server->ops;

        Error_Accept=gui->server->ops->accept(gui->server,gui->client,O_RDWR);

        if (signal_pending(current)) {
                DPRINTK("signal catched!\n");
        }

        if(Error_Accept < 0) {
                DPRINTK(KERN_ALERT " error during accept\n");
                if(gui->client)
                        sock_release (gui->client);
                goto exit;
        }

  /*    if(gui->server->sk->sk_state != TCP_LISTEN){
    PRINT_ERR(KERN_ALERT "tcp dont listen\n");
    sock_release (gui->client);
    goto Accept;
    }*/
        if(gui->server != NULL) {       /*Release old socket*/
                sock_release(gui->server);
                gui->server = NULL;
        }

        if(xit)
                goto exit;

        nread = recv_buffer(gui->client,buffer,60);

	
        if(buffer[0]=='s' || buffer[0]=='S') {
                set_serial_param(buffer);
                sprintf(recv_buff,"SUCCESS");
		//printk("\n[%s]\n\n",buffer);
        }

        else if(buffer[0]=='p' || buffer[0]=='P') {
		set_serial_interface(buffer);
		sprintf(recv_buff,"SUCCESS");
		//printk("\n[%s]\n\n",buffer);
	}
        
        else if(strcmp(buffer,"L i") == 0 || strcmp(buffer,"L I")==0 || (buffer[0]=='L')) {
                get_msr_mcr_stats(recv_buff,gui->portno);
        }
        else if(strcmp(buffer,"E status")==0 || (buffer[0]=='E')) {
               	get_tx_rx_stats(recv_buff,gui->portno);
        }
        else if((strcmp(buffer,"connectiontable")==0) ||(strcmp(buffer,"q")==0))  {
	        get_conn_table(recv_buff, gui->portno);
        }
        else{
                sprintf(recv_buff,"FAILURE");
	}	

        nsend = send_buffer(gui->client,recv_buff,strlen(recv_buff));

        if(gui->client != NULL) {
                sock_release(gui->client);
                gui->client =   NULL;
        }

        goto Accept;

exit:
        if(gui->server !=NULL) {
                DPRINTK("releasing server\n");
                sock_release(gui->server);
                gui->server = NULL;
                kfree(buffer);
                kfree(recv_buff);
                DPRINTK("server released for port %d\n",gui->portno);
        }
        DPRINTK("if_task_gui exited\n");
        complete_and_exit(&gui->gui_complete, 0);
}

void create_gui_thread(struct gui_thrd *gui)
{
        init_completion(&gui->gui_complete);

        gui->pid = kernel_thread(if_task_gui,gui,CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
	num_gui_threads++;

        if(gui->pid < 0) {
                PRINT_ERR("Error During Kernel Thread if_task_gui Creation\n");
                return;
        }
        wait_for_completion (&gui->gui_complete);

}

