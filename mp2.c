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

// !!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!
// Please put your name and email here
MODULE_AUTHOR("Asuthosh Anandaram <aa69@illinois.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS-423 MP2");

#define REGISTER "R"
#define YIELD "Y"
#define DEREGISTER "D"

static struct proc_dir_entry *proc_dir, *proc_entry; 

static LIST_HEAD(pcb_task_list); 

struct kmem_cache *mp2_ts; 

static DEFINE_MUTEX(pcb_list_mutex); 

enum task_state {
    READY,
    RUNNING,
    SLEEPING,
};

struct mp2_task_struct {
	struct task_struct *linux_task;
	struct timer_list wakeup_timer;
	struct list_head list;
	pid_t pid_ts;
	unsigned long period_ms;
	unsigned long computation;
	unsigned long deadline;
	enum task_state state;
};

//void __register_task(char *kbuffer);


#define DEBUG 1
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
    char *pid;
    char *period;
    char *processing_time; 
    char *type;
    char *kbuffer;

	// struct mp2_task_struct *new_task = kmem_cache_alloc(mp2_ts, GFP_KERNEL); 
	// INIT_LIST_HEAD(&new_task->list); 


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

    //printk(KERN_ALERT "Kernel Value Buffer: %s\n", kbuffer);  // Print the raw buffer content


    //Parse the input data (expecting "R,<pid>,<period>,<processing_time>\n")
    type = strsep(&kbuffer, ","); 
	printk(KERN_ALERT "type: %s\n", type);

	//if(type == REGISTER) {
	// printk(KERN_ALERT "type: %s\n", type);

	struct mp2_task_struct *new_task = kmem_cache_alloc(mp2_ts, GFP_KERNEL); 
	INIT_LIST_HEAD(&new_task->list); 
	
  // __register_task(kbuffer + 3); 
    pid = strsep(&kbuffer, ",");
    period = strsep(&kbuffer, ",");
    processing_time = strsep(&kbuffer, ",");

    if (!type || !pid || !period || !processing_time) {
        printk(KERN_ERR "Invalid input format\n");
        kfree(kbuffer);
        return -EINVAL;  // Input format didn't match
    }

    // Log the parsed values
	// printk(KERN_ALERT "Type: %s, PID: %s, Period: %s, Processing Time: %s\n", type, pid, period, processing_time);

	if(kstrtoint(pid, 10, &(new_task->pid_ts)) || kstrtoul(period, 10, &(new_task->period_ms)) || kstrtoul(processing_time, 10, &(new_task->computation))) {
		kfree(kbuffer);
		return -EINVAL; 
	}
	
	//printk(KERN_ALERT "PID: %d, Period: %lu, Processing Time: %lu\n", new_task->pid_ts, new_task->period_ms, new_task->computation);
	
	mutex_lock(&pcb_list_mutex); 
	list_add(&new_task->list, &pcb_task_list); 
	mutex_unlock(&pcb_list_mutex); 

//	}


	//kmem_cache_free(mp2_ts, new_task);
    kfree(kbuffer);  // Free the allocated memory
    return count;
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
	printk(KERN_ALERT "mp2 created....\n"); 

	proc_entry = proc_create("status", 0666, proc_dir, &mp2_ops);

	if (!proc_entry) {
		printk(KERN_ALERT "status creation failed....\n");
		return -ENOMEM;
	}
	printk(KERN_ALERT "status created....\n");

	mp2_ts = kmem_cache_create("mp2_task_struct_cache", sizeof(struct mp2_task_struct), 0, SLAB_PANIC, NULL);


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

	list_for_each_entry_safe(pos, next, &pcb_task_list, list) {  
			printk(KERN_ALERT "Freeing Tasks : %p\n", pos); 
			list_del(&pos->list);
			kmem_cache_free(mp2_ts, pos); 
	}

	remove_proc_entry("status", proc_dir);
	printk(KERN_WARNING "status removed....\n");

	// Remove the directory within the proc filesystem
	remove_proc_entry("mp2", NULL);
	printk(KERN_WARNING "mp2 removed...\n");
	
	kmem_cache_destroy(mp2_ts); 

	printk(KERN_ALERT "RTS MODULE UNLOADED\n");

	
}

// Register init and exit funtions
module_init(rts_init);
module_exit(rts_exit);
