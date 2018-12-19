#include "proc.h"

ssize_t ss_procfile_read(  char* buffer,
                        char** buffer_location,
                        off_t offset,
                        int buffer_length,
                        int* eof,
                        void* data)
{
        int len =0;

        if(offset > 0){
                *eof = 1;
                return len;
        }
	//printk("number_of_ports = %d\n",number_of_ports);
        len = sprintf(buffer,"%d",number_of_ports);
        return len;
}

ssize_t ss_procfile_write( struct file* file,
                        const char* buffer,
                        unsigned long count,
                        void* data)
{

        /*if(count<(MAX_BUFSIZE-1)){
                strncpy(sock_port1,buffer,count);
                sock_port1[count] = '\0';
        }*/

        return count;
}

