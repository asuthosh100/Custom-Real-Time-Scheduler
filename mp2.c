#define LINUX
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>   
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h> 
#include <linux/workqueue.h> 
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/jiffies.h>  
#include "mp2_given.h"
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>



// !!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!
// Please put your name and email here
MODULE_AUTHOR("Asuthosh Anandaram <aa69@illinois.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS-423 MP2");

#define REGISTER "R"
#define YIELD "Y"
#define DEREGISTER "D"

#define DEBUG 1
#define BOUND 6930
#define FIXED_POINT_SCALE 10000

static struct proc_dir_entry *proc_dir, *proc_entry; 

static LIST_HEAD(pcb_task_list); 

struct kmem_cache *mp2_ts; 

static DEFINE_MUTEX(pcb_list_mutex); 
static DEFINE_MUTEX(pcb_ts_mutex);

enum task_state {
    READY,
    RUNNING,
    SLEEPING,
};

static struct timer_list mp2_timer;

struct mp2_task_struct {
	struct task_struct *linux_task;
	struct timer_list wakeup_timer;
	struct list_head list;
	unsigned int pid_ts;
	unsigned long period_ms;
	unsigned long computation;
	unsigned long deadline;
	enum task_state state;
};

struct task_struct *dispatcher_thread_struct; 

struct mp2_task_struct* mp2_current;



void __sched_new_task(struct mp2_task_struct *task);
void __sched_old_task(struct mp2_task_struct *tsk);
void __wake_up_current_task(struct mp2_task_struct *data);
void timer_callback(struct timer_list *timer);
static int admission_control(struct mp2_task_struct *data);

// void register_task(unsigned int pid, unsigned long period, unsigned long computation);
// void deregister_task(unsigned int pid);
// void yield_handler(unsigned int pid);

void register_task(char *kbuf);
void deregister_task(char *kbuf);
void yield_handler(char *kbuf);



//------------------------------------------------------------------
static ssize_t read_handler(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) 
{	
	//printk(KERN_ALERT "read_handler"); 

	struct mp2_task_struct *p; 
	char *kbuf; 
	int len = 0; 
	//ssize_t ret = len; 
	//unsigned long flags; 

	kbuf = (char *)kmalloc(count, GFP_KERNEL); 

	if(!kbuf) {
		return -ENOMEM;
	}

	// traverse over the list and read the current cpu time of the pid.  
	
	mutex_lock(&pcb_list_mutex);
	list_for_each_entry(p, &pcb_task_list, list) {
		len += sprintf(kbuf + len, "%u, %lu, %lu\n", p->pid_ts, p->period_ms, p->computation);
		//printk(KERN_INFO "PID:%d and READ_TIME:%lu\n", p->pid, p->cpu_time);
		if(len > count) {
	        len = count;
	        break;
	  }
	}
	mutex_unlock(&pcb_list_mutex); 
	
	//printk(KERN_ALERT "Kbuf value in Read handler: %s", kbuf);

	
	//checks bounds of len
	if(len > count) {
	  	len = count;
	}
  
    if (len < count) {
	  kbuf[len] = '\0';
	}

	if(*ppos >= len) {
		kfree(kbuf);
		return 0;
    }
    // send it to user buffer
    if (copy_to_user(ubuf, kbuf, len)) {
		kfree(kbuf);
    	return -EFAULT;
	}
    
	//update *ppos according to len
    *ppos += len;
	kfree(kbuf);

	// return bytes read
	return len;

}

static ssize_t write_handler(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) 
{	

    //char *type;
    char *kbuffer;
	// char *kbuf_copy;
	// // unsigned int pid;
	// // unsigned long period;
	// // unsigned long computation;


    // Allocate memory for the kernel buffer
    kbuffer = kmalloc(count + 1, GFP_KERNEL);
	
    if (!kbuffer) {
        return -ENOMEM;
    }

    // Copy data from user space to the kernel buffer
    if (copy_from_user(kbuffer, ubuf, count)) {
        printk(KERN_ALERT "invalid copy data");
        kfree(kbuffer);
        return -EFAULT; 
    }

    kbuffer[count] = '\0';  // Null-terminate the string


	if(kbuffer[0] == 'R') {
		register_task(kbuffer);
	}

	else if(kbuffer[0] == 'Y') {
		yield_handler(kbuffer);
	}

	else if(kbuffer[0] == 'D') {
		deregister_task(kbuffer);
	}

	// kfree(kbuf_copy);
	kfree(kbuffer);  // Free the allocated memory
    return count;
}


void register_task(char *kbuf) {
	struct mp2_task_struct *new_task = kmem_cache_alloc(mp2_ts, GFP_KERNEL); 
	if (!new_task) {
        printk(KERN_ERR "Failed to allocate memory for new task\n");
        return;
    }
	INIT_LIST_HEAD(&new_task->list); 

	sscanf(kbuf, "R,%u,%lu,%lu", &new_task->pid_ts, &new_task->period_ms, &new_task->computation);
	//printk(KERN_ALERT "Parsed PID in kernel: %u\n", new_task->pid_ts);


	new_task->deadline = 0; 

	new_task->state = SLEEPING;


	//printk(KERN_ALERT "pid in kernel %d\n", pid_for_lt );

	new_task->linux_task = find_task_by_pid(new_task->pid_ts); 

	if (new_task->linux_task == NULL) {
    printk(KERN_ERR "Failed to find task with PID: %d\n", new_task->pid_ts);
    //kmem_cache_free(mp2_ts, new_task); // example cleanup
    return;

	} else {
    printk(KERN_INFO "Found task with PID: %d\n", new_task->pid_ts);
 }

	timer_setup(&new_task->wakeup_timer, timer_callback, 0); 

	if (admission_control(new_task) == 1) {
		//kmem_cache_free(mp2_ts, new_task);
        return;
    }
	
	//printk(KERN_ALERT "PID: %d, Period: %lu, Processing Time: %lu\n", new_task->pid_ts, new_task->period_ms, new_task->computation);
	
	mutex_lock(&pcb_list_mutex); 
	list_add(&new_task->list, &pcb_task_list); 
	mutex_unlock(&pcb_list_mutex); 
}

void deregister_task(char *kbuf) {

 	struct mp2_task_struct *pos, *next; 
	unsigned int pid;

	printk(KERN_ALERT "Deregister handler invoked\n");

	sscanf(kbuf, "D,%u", &pid);
	//printk(KERN_ALERT "Parsed PID in kernel: %u\n", pid);

	mutex_lock(&pcb_list_mutex); 
	list_for_each_entry_safe(pos, next, &pcb_task_list, list) {
		if(pos->pid_ts == pid) { 
			del_timer(&pos->wakeup_timer);
			list_del(&pos->list);
			printk(KERN_ALERT "Deregistering pid %d\n",pid);

			if(mp2_current == pos) {
				mp2_current = NULL;
			}
			kmem_cache_free(mp2_ts, pos);
			
			wake_up_process(dispatcher_thread_struct);
			break;
		}
	
	}
	mutex_unlock(&pcb_list_mutex);



}


void yield_handler(char *kbuf) {

	struct mp2_task_struct *pos,*next;
	struct mp2_task_struct *req_task;

	unsigned int pid;

	sscanf(kbuf, "Y,%u", &pid);
	//printk(KERN_ALERT "Parsed PID in kernel: %u\n", pid);

	printk(KERN_ALERT "entering yield handler");


	printk(KERN_ALERT "yield handler:pid is %d\n", pid);


	//printk(KERN_ALERT "locking facility for pcb list mutex (yield handler)");

	// find the calling task
	mutex_lock(&pcb_list_mutex); 
	list_for_each_entry_safe(pos,next,&pcb_task_list,list) {
		if(pos->pid_ts == pid) {
			req_task = pos; 
			break;
		}
	}
	mutex_unlock(&pcb_list_mutex);

	 if (!req_task) {
        printk(KERN_ERR "Task with PID %u not found\n", pid);
        return;
    }

	if(req_task->deadline == 0) {
		req_task->deadline = jiffies + msecs_to_jiffies(req_task->period_ms);
		//printk(KERN_ALERT "yield initial deadline : %lu and pid %d\n", req_task->deadline, req_task->pid_ts);
		//req_task->state = READY;
	} else {
		req_task->deadline = req_task->deadline + msecs_to_jiffies(req_task->period_ms);
		//printk(KERN_ALERT "yield additional deadline : %lu and pid %d\n", req_task->deadline, req_task->pid_ts);
		req_task->state = SLEEPING;

		if(req_task->deadline < jiffies) {
			return;
		}

		// else {
		// 	break;
		// }
	}

	

	mod_timer(&(req_task->wakeup_timer), req_task->deadline);
	// req_task->state = SLEEPING; 

	wake_up_process(dispatcher_thread_struct);

    set_current_state(TASK_UNINTERRUPTIBLE);

	schedule();

//TASK_INTERRUPTIBLEs
}

void timer_callback(struct timer_list *timer) {

	struct mp2_task_struct *task;

	mutex_lock(&pcb_ts_mutex); 
	printk(KERN_ALERT "Timer CallBack Invoked\n");
    task = from_timer(task, timer, wakeup_timer);
	printk(KERN_ALERT "pid at the timer callback : %d\n", task->pid_ts);
    task->state = READY;
	mutex_unlock(&pcb_ts_mutex); 

    wake_up_process(dispatcher_thread_struct);
}




void __sched_old_task(struct mp2_task_struct *task) {

	struct sched_attr attr;
    if (!task || !task->linux_task) {
        printk(KERN_ERR "Invalid task or task->linux_task in __sched_old_task\n");
        return;
    }
	printk(KERN_ALERT "pid of task to be scheduled old_task is %d\n", task->pid_ts);
    // struct sched_attr attr;
    attr.sched_policy = SCHED_NORMAL;
    attr.sched_priority = 0;
    sched_setattr_nocheck(task->linux_task, &attr);
}

void __sched_new_task(struct mp2_task_struct *task) {
	struct sched_attr attr; 
    if (!task || !task->linux_task) {
        printk(KERN_ERR "Invalid task or task->linux_task in __sched_new_task\n");
        return;
    }
	printk(KERN_ALERT "pid of task to be scheduled new_task is %d\n", task->pid_ts);
    // struct sched_attr attr; 
    attr.sched_policy = SCHED_FIFO;
    attr.sched_priority = 99;
    sched_setattr_nocheck(task->linux_task, &attr);
}


static int dispatching_thread(void *data) {
    
	struct mp2_task_struct *pos, *next, *temp;
	printk(KERN_ALERT "Dispatcher thread started");

    while (!kthread_should_stop()) {

		set_current_state(TASK_INTERRUPTIBLE);
        schedule(); 
		// Sleep if no tasks are `READY`
        // Check if any task is ready
		mutex_lock(&pcb_ts_mutex);

        temp = NULL;

		
		//printk(KERN_ALERT "Entering Dispatcher thread pcbTaskList lock\n");
        mutex_lock(&pcb_list_mutex);
        list_for_each_entry_safe(pos, next, &pcb_task_list, list) {
            if (pos->state == READY) {
                if (!temp || pos->period_ms < temp->period_ms) {
                    temp = pos;
                }
            }
        }
        mutex_unlock(&pcb_list_mutex);
		//printk(KERN_ALERT "Exiting Dispatcher thread pcbTaskList lock\n");

		//mutex_lock(&pcb_ts_mutex);

		if(temp == NULL) {
			if(mp2_current) {
				 __sched_old_task(mp2_current);
			}
		}

		else {
			 if (mp2_current && mp2_current->period_ms > temp->period_ms) {
                // Demote the current task
                mp2_current->state = READY;
				__sched_old_task(mp2_current); 
            }
            __wake_up_current_task(temp); // Promote the new task
		}

		mutex_unlock(&pcb_ts_mutex);

        // set_current_state(TASK_INTERRUPTIBLE);
        // schedule(); // Sleep if no tasks are `READY`
    }

    printk(KERN_ALERT "Dispatcher thread stopping\n");
    return 0;
}


static int admission_control(struct mp2_task_struct *data) {

	struct mp2_task_struct *pos, *next; 
	unsigned long total_util = 0;
	

	mutex_lock(&pcb_list_mutex);
	list_for_each_entry_safe(pos, next, &pcb_task_list, list) {
		total_util += (pos->computation * FIXED_POINT_SCALE) / pos->period_ms; 
	}


	total_util += (data->computation * FIXED_POINT_SCALE)/data->period_ms ;

	mutex_unlock(&pcb_list_mutex);

	if(total_util > BOUND) {
		return 1;
	}

	else {
		return 0; 
	}
}


void __wake_up_current_task(struct mp2_task_struct *data) {
	
	wake_up_process(data->linux_task);
	printk(KERN_ALERT "WAKE UP CURRENT TASK with pid :%d", data->pid_ts);
	__sched_new_task(data);
	data->state = RUNNING; 
	mp2_current = data; 
	printk(KERN_ALERT "Current Running Task PID %d", mp2_current->pid_ts); 
}


static const struct proc_ops mp2_ops = 
{	
	.proc_open = simple_open,
	.proc_read = read_handler,
	.proc_write = write_handler,
};


// mp2_init - Called when module is loaded
int __init rts_init(void)
{
#ifdef DEBUG
	printk(KERN_ALERT "RTS MODULE LOADING\n");
#endif
	// Insert your code here ...
	proc_dir = proc_mkdir("mp2", NULL);
	if (!proc_dir) {
		printk(KERN_ALERT "Failed to create proc directory\n");
		return -ENOMEM;
	}

	proc_entry = proc_create("status", 0666, proc_dir, &mp2_ops);
	if (!proc_entry) {
		printk(KERN_ALERT "Failed to create proc entry\n");
		remove_proc_entry("mp2", NULL);
		return -ENOMEM;
	}

	printk(KERN_ALERT "status created....\n");

	mp2_ts = kmem_cache_create("mp2_task_struct_cache", sizeof(struct mp2_task_struct), 0, SLAB_HWCACHE_ALIGN, NULL);

	//mp2_ts = kmem_cache_create("mp2_task_struct_cache", sizeof(struct mp2_task_struct), 0, SLAB_PANIC | __GFP_NOWARN, NULL);

	dispatcher_thread_struct = kthread_run(dispatching_thread, NULL, "dispatching thread"); 

	printk(KERN_ALERT "RTS MODULE LOADED\n");



	return 0;
}

// mp2_exit - Called when module is unloaded
void __exit rts_exit(void)
{

	struct mp2_task_struct *pos, *next;
	#ifdef DEBUG
		printk(KERN_ALERT "RTS MODULE UNLOADING\n");
	#endif



	

	del_timer_sync(&mp2_timer);

	kthread_stop(dispatcher_thread_struct);

	mutex_destroy(&pcb_list_mutex);
	mutex_destroy(&pcb_ts_mutex); 

	list_for_each_entry_safe(pos, next, &pcb_task_list, list) {  
			printk(KERN_ALERT "Freeing Tasks : %p\n", pos); 
			list_del(&pos->list);
			kmem_cache_free(mp2_ts, pos); 
	}
	
	kmem_cache_destroy(mp2_ts); 

	remove_proc_entry("status", proc_dir);
	printk(KERN_WARNING "status removed....\n");

	// Remove the directory within the proc filesystem
	remove_proc_entry("mp2", NULL);
	printk(KERN_WARNING "mp2 removed...\n");

	printk(KERN_ALERT "RTS MODULE UNLOADED\n");

	
}

// Register init and exit funtions
module_init(rts_init);
module_exit(rts_exit);
