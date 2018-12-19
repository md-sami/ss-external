
#include "serialserver.h"
#include "debug.h"
#include "sockets.h"
#include "timers.h"

/*
 *  This Function Will enable the Inactivity Timer
 *  - initialize structure and the timers internal values.
 *  - activate the timer using add_timer()
 */

void Enable_Inactivity_timer(nw2serial_t * wap , int timer_val)
{
        init_timer(&wap->watchdog_timer);  /*initializing the timer structure */
        wap->watchdog_timer.expires = jiffies + timer_val*HZ;
        wap->watchdog_timer.function = &Release_Socket;
        wap->watchdog_timer.data =(unsigned long) wap;
        add_timer(&wap->watchdog_timer); /* insert my timer into the global list of active timers*/
}

/*
 * This Function will deactivate the inactivity timer prior to its 
 * exipiration set in Enable_Inactivity_Timer function.
 */
void Disable_Inactivity_timer(nw2serial_t * wap)
{
        del_timer(&wap->watchdog_timer);
}

/*
 * This function will modify the expiration of an already active timer.
 * this is done using the function mod_timer which changes the expiration of
 * a give timer.
 */
void Modify_Inactivity_timer(nw2serial_t * wap)
{
	if((wap->InActivity_Time != 0) && (wap->InActivity_Disabled==1))
	//if((wap->InActivity_Time != 0))
	{
        	mod_timer(&wap->watchdog_timer,jiffies + wap->InActivity_Time*HZ);
	}
}	

/*
 * This function sets the keepalive time  required for tcpserver mode and 
 * tcpclient mode
 */

void setkeepalive(nw2serial_t *wap,struct socket *sockfd)
{
        mm_segment_t oldfs;
        int keep_idle,keep_cnt,alive_time;

        oldfs = get_fs(); set_fs(get_ds());
        if(wap->Keep_Alive_Time) {
                keep_idle =15;
                /* Keep the window size 1.5K to avoid sender flooding us */
               /* window_size=1536;
                if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_WINDOW_CLAMP,
                                        (char __user *)&window_size,sizeof(window_size))) {
                        printk("error in setting TCP_WINDOW_CLAMP\n");
                }*/
                if(sock_setsockopt(sockfd, SOL_SOCKET,SO_KEEPALIVE,
                                        (char __user *)&wap->Keep_Alive_Time, sizeof(wap->Keep_Alive_Time))) {
                        printk(KERN_ERR "error: keep alive time = %d\n",wap->Keep_Alive_Time);
                }
                if(wap->Keep_Alive_Time == 3) {
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&keep_idle,sizeof(keep_idle))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
                }
                else if(wap->Keep_Alive_Time == 2) {
                        keep_cnt = 2;
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&keep_idle,sizeof(keep_idle))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPCNT,
                                        (char __user *)&keep_cnt,sizeof(keep_cnt))) {
                                printk("error in setting TCP_KEEPCNT\n");
                        }
                }
                else if(wap->Keep_Alive_Time == 1){
                        keep_cnt=1;
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&keep_idle,sizeof(keep_idle))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
			   if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPCNT,
                                        (char __user *)&keep_cnt,sizeof(keep_cnt))) {
                                printk("error in setting TCP_KEEPCNT\n");
                        }			

                }
                else {
                        alive_time = (wap->Keep_Alive_Time-3)*60;
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&alive_time,sizeof(alive_time))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
                }

        }
        set_fs(oldfs);
}


/*
 * This function sets the zero inactivity time  required for tcpserver mode.
 */

void setkeepalive_zero_inactivity(nw2serial_t *wap,struct socket *sockfd)
{
        mm_segment_t oldfs;
        int keep_idle,keep_cnt;//,alive_time;
	int keep_intvl=20;

        oldfs = get_fs(); set_fs(get_ds());
        //if(wap->Keep_Alive_Time) {
                keep_idle =10;
                /* Keep the window size 1.5K to avoid sender flooding us */
               /* window_size=1536;
                if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_WINDOW_CLAMP,
                                        (char __user *)&window_size,sizeof(window_size))) {
                        printk("error in setting TCP_WINDOW_CLAMP\n");
                }*/
                //if(wap->Keep_Alive_Time == 0){
                        keep_cnt=1;
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&keep_idle,sizeof(keep_idle))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
			
			if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPCNT,
                                        (char __user *)&keep_cnt,sizeof(keep_cnt))) {
                                printk("error in setting TCP_KEEPCNT\n");
                        }			
			
			if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPINTVL,
                                        (char __user *)&keep_intvl,sizeof(keep_intvl))) {
                                printk("error in setting TCP_KEEPINTVL\n");
                        }			

                //}
                /*else {
                        alive_time = (wap->Keep_Alive_Time-3)*60;
                        if(sockfd->ops->setsockopt(sockfd,IPPROTO_TCP,TCP_KEEPIDLE,
                                        (char __user *)&alive_time,sizeof(alive_time))) {
                                printk("error in setting TCP_KEEPIDLE\n");
                        }
                }*/

        //}
        set_fs(oldfs);
}



