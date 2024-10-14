#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/init.h>

#include "mp2_given.h"

// !!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!
// Please put your name and email here
MODULE_AUTHOR("Asuthosh Anandaram <aa69@illinois.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS-423 MP2");

static struct proc_dir_entry *proc_dir, *proc_entry; 


#define DEBUG 1
//------------------------------------------------------------------
static ssize_t myread(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) 
{
	printk( KERN_DEBUG "read handler\n");
	return 0;
}

static ssize_t mywrite(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) 
{
	printk( KERN_DEBUG "write handler\n");
	return 0;
	
}


static const struct proc_ops mp1_ops = 
{
	.proc_open = simple_open,
	.proc_read = myread,
	.proc_write = mywrite,
};


// mp2_init - Called when module is loaded
int __init rts_init(void)
{
#ifdef DEBUG
	printk(KERN_ALERT "RTS MODULE LOADING\n");
#endif
	// Insert your code here ...

	proc_dir = proc_mkdir("mp2", NULL);
	printk(KERN_ALERT "mp1 created....\n"); 

	proc_entry = proc_create("status", 0666, proc_dir &mp1_ops);

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
