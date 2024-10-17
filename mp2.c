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

static struct proc_dir_entry *proc_dir, *proc_entry; 

// struct mp2_task_struct {
// 	struct task_struct *linux_task;
// 	struct timer_list wakeup_timer;
// 	struct list_head list;
// 	pid_t pid;
// 	unsigned long period;
// 	unsigned long processing_time;
// 	unsigned long deadline_jiff;
// 	enum task_state state;
// };


#define DEBUG 1
//------------------------------------------------------------------
static ssize_t read_handler(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) 
{
	printk( KERN_DEBUG "read handler\n");
	return 0;
}

static ssize_t write_handler(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) 
{	
    char *pid;
    char *period;
    char *processing_time; 
    char *type;
    char *kbuffer;

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

    printk(KERN_ALERT "Kernel Value Buffer: %s\n", kbuffer);  // Print the raw buffer content

    //Parse the input data (expecting "R,<pid>,<period>,<processing_time>\n")
    type = strsep(&kbuffer, ","); 
    pid = strsep(&kbuffer, ",");
    period = strsep(&kbuffer, ",");
    processing_time = strsep(&kbuffer, ",");

    if (!type || !pid || !period || !processing_time) {
        printk(KERN_ERR "Invalid input format\n");
        kfree(kbuffer);
        return -EINVAL;  // Input format didn't match
    }

    // Log the parsed values
    printk(KERN_ALERT "Type: %s, PID: %s, Period: %s, Processing Time: %s\n", type, pid, period, processing_time);

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


	printk(KERN_ALERT "RTS MODULE LOADED\n");
	return 0;
}

// mp2_exit - Called when module is unloaded
void __exit rts_exit(void)
{
#ifdef DEBUG
	printk(KERN_ALERT "RTS MODULE UNLOADING\n");
#endif
	// Insert your code here ...

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
