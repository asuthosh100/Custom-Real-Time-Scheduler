[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/bN8FEXqy)
# UIUC CS 423 MP2

Your Name: Asuthosh Anandaram

Your NetID: aa69 

How to Test :

*  make
* insmod mp2.ko
* ./userapp 4 2 3 
(here 4 is the number of iterations the process will run for, 2 is number of the times will we computer the factorial, and 3 gets multiplied with the computation to form the period). 

* You can run this : ./userapp 4 2 3 & ./userapp 3 2 5 

Documentation 

The MP involves implementing a Rate-Monotonic Scheduler in the Linux kernel. It uses the Periodic Task Model from Liu and Layland in handling real-time CPU scheduling. The RMS allows tasks to be scheduled according to their periodic intervals. The higher the priority, the shorter the period will be, which enables efficient preemption and context switching in order for those tasks to complete on time. RMS is implemented as a kernel module exporting the possibility to perform registration, yielding, and de-registration through the /proc/mp2/status interface, allowing userspace to interact with kernel scheduling parameters.

Admission control ensures that all tasks cannot exceed a fixed utilization bound; therefore, no deadline can be missed. Another important constituent of it is the slab allocator, which permits efficient memory management inside the kernel by the operating system for better performance. Here, the scheduler maintains each task state in its PCB with its scheduling parameters and a wake-up timer. These enable it to manage tasks in READY, RUNNING, and SLEEPING states. The RMS will be tested by a test application running from user-space that would register, yield, and deregister tasks to simulate a real-time periodic load to validate the behavior of the scheduler. This project will delve into the understanding of the Linux scheduler APIs, kernel memory management, and fixed-point arithmetic for real-time system performance by following best practices in kernel development and software engineering.
