#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// #include <lib.h>

#define FILE_PATH "/proc/mp2/status"
#define SIZE 100
#define PIDSIZE 100

//=============================================================================

//long getTime_ms(struct timespec begin);
int process_in_the_list(unsigned int pid, int pid_arr[], int size);
void do_job();
void yield(int fd, unsigned int pid);
//void deregister(int fd, char pid_str);
void deregister(int fd, unsigned int pid);
unsigned long get_time_ms(struct timespec time);

//==================================================================================

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    char pid_str[10]; // Buffer to hold the string representation of the PID
    sprintf(pid_str, "%d", pid); // Convert the integer PID to a string

    //printf("Successfully registered process with PID: %s\n", pid_str);

    int fd; 
    char buffer[SIZE];
    char rbuf[SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <period> <processing_time>\n", argv[0]);
        return 1;
    }

    //char *period = argv[1];
    //char *processing_time = argv[2];

    int yield_iterations = atoi(argv[2]);
    int period_rand = atoi(argv[1]);


   //printf("Per: %s and Comp: %s\n", period, processing_time);
   
   struct timespec start_time, time_after_call;
    int computation;

    
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    
    do_job(); 

    clock_gettime(CLOCK_MONOTONIC, &time_after_call);

    computation = (int)(get_time_ms(time_after_call) - get_time_ms(start_time))/1000;

    printf("computation : %lu\n", computation);

    int period = period_rand*computation;

    printf("period = %d", period);


//-------------REGISTER-------------------------------

    //Open the file directly for writing
    fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return 1;
    }

    // Format the registration string
    memset(buffer, 0, SIZE); // Clear the buffer
    sprintf(buffer, "R,%s,%d,%d\n", pid_str, period, computation);

    //printf("Buffer val: %s\n", buffer);
    // Write the string to /proc/mp2/status
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write to /proc/mp2/status");
        close(fd);
        return 1;
    }

//---------------------YIELD---------------------------------------------------------------------------

    memset(rbuf, 0, sizeof(rbuf));
    read(fd, rbuf, sizeof(rbuf));
    //printf("Read Buffer val: %s\n", rbuf);
    //puts(rbuf);
    
    unsigned long r_period, r_computation; 
    unsigned int r_pid; 
    char *tokenize; 
    int pid_array[PIDSIZE]; 


    tokenize = strtok(rbuf, "\n"); 
    int i = 0;

    while(tokenize != NULL) {
        if(sscanf(tokenize, "%d, %lu, %lu", &r_pid, &r_period, &r_computation)){
                //printf("TOKENIZED %d, %lu, %lu\n", r_pid, r_period, r_computation );
                pid_array[i] = r_pid;
        
                i++;
        }

        else {
             printf("Error: Unable to parse token: %s\n", tokenize);
        }

        tokenize = strtok(NULL, "\n"); 
    }

    printf("pid array: ");
    for (int j = 0; j < i; j++) {
    printf("%d ", pid_array[j]);
    }
    printf("\n");

    printf("Checking for process in the list\n"); 

    int proc = process_in_the_list((unsigned int)pid, pid_array, i);

   // printf("proc val %d\n", proc); 

    if(proc == 0) {
        close(fd);
        return 0;
    }

    //printf("Line 105");

    // long wakeup_time, process_time; 
    // struct timespec t0; 

    // clock_gettime(CLOCK_MONOTONIC, &t0); 

    yield(fd, r_pid); 
    printf("Initial Yield done\n") ; 


    printf("entering while loop for running the process\n");
    while(yield_iterations--) {

        //wakeup_time = getTime_ms(t0); 

        printf("doing job");

        do_job(); 
        //usleep(10000); 

        printf("job done");

        //process_time = getTime_ms(t0) - wakeup_time; 

       yield(fd, r_pid); 

       //usleep(10000); 

    }
    //-----------DEREGISTER--
    //------------------------------------------------------------------------------------
    // Close the file

    deregister(fd, r_pid);

    close(fd);

    //printf("Successfully registered process with PID: %s\n", pid_str);
    return 0;
}

void yield(int fd, unsigned int pid) {

    char ybuf[SIZE];
    memset(ybuf, 0, SIZE); 
    sprintf(ybuf, "Y,%d\n",pid);

    write(fd, ybuf, strlen(ybuf));

}

void deregister(int fd, unsigned int pid) {

    char dbuf[SIZE];
    memset(dbuf, 0, SIZE); 
    sprintf(dbuf, "D,%d\n",pid);

    printf("Dbuf %s\n", dbuf);

    write(fd, dbuf, strlen(dbuf));
}

void do_job() {
    volatile long long unsigned int sum = 0;
    for (int i = 0; i < 1000; i++) {
        volatile long long unsigned int fac = 1;
        for (int j = 1; j <= 10; j++) {
            fac *= j;
        }
        sum += fac;
    }
}


int process_in_the_list(unsigned int pid, int pid_arr[], int size) {
    for(int i = 0; i<size; i++) {
        if(pid_arr[i] == pid) {
            printf("Process in the list helper, pid: %d\n", pid);
            return 1;
        }   

        else {
            continue;
        }  
    }
    return 0;
}

// long getTime_ms(struct timespec begin) {
//     struct timespec curr; 
//     long sec, nanosec; 

//     clock_gettime(CLOCK_MONOTONIC, &curr); 

//     sec = curr.tv_sec - begin.tv_sec;
//     nanosec = curr.tv_nsec - begin.tv_nsec; 

//     return (sec*1000) + (nanosec/1000000); 
// }

unsigned long get_time_ms(struct timespec time) {
     return time.tv_sec * 1000000000L + time.tv_nsec;
}