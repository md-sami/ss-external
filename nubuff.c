/**
 * ------------
 * nubuff.c
 * ------------
 * Project Name : MCS8140-SerialServer-2.1
 * File Revision: 1.1
 * Author       : from excelstore
 * Date Created : Thu Apr 3 2018
 *
 * Modification History:
 *         Id  Date                By      Description
 * $Id: nubuff.c,v 1.1 2018/06/28 11:46:20 sami Exp $
 *
 **/
#include <linux/skbuff.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/interrupt.h>
#include <linux/cache.h>
#include "nubuff.h"
#include "debug.h"

struct nu_buff * dev_alloc_nub(unsigned int size)
{
   	struct nu_buff *nub;
   	nub = kmalloc(sizeof(struct nu_buff),GFP_ATOMIC);

	if(!nub) {
   		DPRINTK("nub alloc failed\n");
   		return NULL;
   	}

	nub->data  = kzalloc(size,GFP_ATOMIC);

	if(!(nub->data)) {
   		DPRINTK("nub data  alloc failed\n");
		kfree(nub);
		return NULL;
   	}
	
	nub->buffer  = kzalloc(size,GFP_ATOMIC);

	if(!(nub->buffer)) {
   		DPRINTK("nub data  alloc failed\n");
		kfree(nub);
		return NULL;
   	}
	
  	return nub;
}

 void dev_free_nub(struct nu_buff *nub)
{
//	printk("in dev free nub\n");
#if 1
	if(nub) {
      		if(nub->data != NULL) { 
	  		kfree(nub->data);
        	}
      		if(nub->buffer != NULL) { 
	  		kfree(nub->buffer);
        	}
		kfree(nub);
    	}
	//printk("end of dev free nub\n");
#endif
}


struct nu_buff * dev_alloc_rx_nub(unsigned int size)
{
        struct nu_buff *nub;
        nub = kmalloc(sizeof(struct nu_buff),GFP_ATOMIC);

        if(!nub) {
                DPRINTK("nub alloc failed\n");
                return NULL;
        }

        nub->data  = kzalloc(size,GFP_ATOMIC);
	
        if(!(nub->data)) {
                DPRINTK("nub data  alloc failed\n");
                kfree(nub);
                return NULL;
        }
	nub->buffer = NULL;

        return nub;
}

 void dev_free_rx_nub(struct nu_buff *nub)
{
        if(nub) {
                if(nub->data != NULL) {
                        kfree(nub->data);
                }
                kfree(nub);
        }
}



int nub_queue_empty(const struct nu_buff_head *list)
{
	return list->next == (struct nu_buff *)list;
}

void __nub_unlink(struct nu_buff *nub, struct nu_buff_head *list)
{
	struct nu_buff *next, *prev;
	list->qlen--;
	next	   = nub->next;
	prev	   = nub->prev;
	nub->next  = nub->prev = NULL;
        next->prev = prev;
        prev->next = next;
}

struct nu_buff *__nub_dequeue(struct nu_buff_head *list)
{
	struct nu_buff *next, *prev, *result;

	prev = (struct nu_buff *) list;
	next = prev->next;
	result = NULL;
	if (next != prev) {
		result	     = next;
		next	     = next->next;
		list->qlen--;
		next->prev   = prev;
		prev->next   = next;
		result->next = result->prev = NULL;
	}
	return result;
}

struct nu_buff *nub_dequeue(struct nu_buff_head *list)
{
	unsigned long flags;
	struct nu_buff *result;

	spin_lock_irqsave(&list->lock, flags);
	result = __nub_dequeue(list);
	spin_unlock_irqrestore(&list->lock, flags);
	return result;
}

void __nub_queue_tail(struct nu_buff_head *list,
				   struct nu_buff *newsk)
{
	struct nu_buff *prev, *next;
	list->qlen++;
	next = (struct nu_buff *)list;
	prev = next->prev;
	newsk->next = next;
	newsk->prev = prev;
	next->prev  = prev->next = newsk;
}

void nub_queue_tail(struct nu_buff_head *list, struct nu_buff *newsk)
{
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);
	__nub_queue_tail(list, newsk);
	spin_unlock_irqrestore(&list->lock, flags);
}

__u32 nub_queue_len(const struct nu_buff_head *list_)
{
	return list_->qlen;
}

void nub_queue_head_init(struct nu_buff_head *list)
{
	spin_lock_init(&list->lock);
	list->prev = list->next = (struct nu_buff *)list;
	list->qlen = 0;
}

