#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// #include <lib.h>

#define FILE_PATH "/proc/mp2/status"
#define SIZE 1000
#define PIDSIZE 1000

//=============================================================================

//long getTime_ms(struct timespec begin);
int process_in_the_list(unsigned int pid, int pid_arr[], int size);
void do_job();
void yield(unsigned int pid);
void deregister(unsigned int pid);
unsigned long get_time_ms(struct timespec time);
void register_task(unsigned int pid, unsigned long period, unsigned long computation);
int check_status(int pid);
//==================================================================================

int main(int argc, char *argv[])
{
    unsigned int pid = getpid();
    // char pid_str[10]; // Buffer to hold the string representation of the PID
    // sprintf(pid_str, "%d", pid); // Convert the integer PID to a string

    //printf("Successfully registered process with PID: %d\n", pid);

    char rbuf[SIZE];

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <period> <processing_time>\n", argv[0]);
        return 1;
    }

    //char *period = argv[1];
    //char *processing_time = argv[2];

    int yield_iterations = atoi(argv[1]); // for how many periods
    int jobs = atoi(argv[2]); // number of jobs
    int period_rand = atoi(argv[3]); // for period time


   //printf("Per: %s and Comp: %s\n", period, processing_time);
   
   struct timespec start_time, time_after_call;
    unsigned long computation;

    
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    
    do_job(jobs); 

    clock_gettime(CLOCK_MONOTONIC, &time_after_call);

    computation = (unsigned long)(get_time_ms(time_after_call) - get_time_ms(start_time))/1000;

    //printf("computation : %lu\n", computation);

    unsigned long period = (unsigned long)period_rand*computation;

   // printf("period = %d", period);


//-------------REGISTER-------------------------------
    register_task(pid,period,computation);

//---------------------YIELD---------------------------------------------------------------------------

   if(check_status(pid) == 0) {
        return 0; 
   }


    yield(pid); 
   
    while(yield_iterations--) {

        printf("doing job with pid %d\n", pid);

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        do_job(jobs); 
       
        printf("job done with pid %d\n", pid);


       yield(pid); 


    }
    //-----------DEREGISTER--
    //------------------------------------------------------------------------------------
    // Close the file

    deregister(pid);


    //printf("Successfully registered process with PID: %s\n", pid_str);
    return 0;
}

int check_status(int pid) {

    char rbuf[SIZE];
    int fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return 0;
    }

    memset(rbuf, 0, sizeof(rbuf));
    read(fd, rbuf, sizeof(rbuf));
    printf("Read Buffer val: %s\n", rbuf);
    //puts(rbuf);
    
    unsigned long r_period, r_computation; 
    unsigned int r_pid; 
    char *tokenize; 
    int pid_array[PIDSIZE]; 


    tokenize = strtok(rbuf, "\n"); 
    int i = 0;

    while(tokenize != NULL) {
        if(sscanf(tokenize, "%d: %lu, %lu", &r_pid, &r_period, &r_computation)){
                printf("TOKENIZED %d: %lu, %lu\n", r_pid, r_period, r_computation );
                pid_array[i] = r_pid;
        
                i++;
        }

        else {
             printf("Error: Unable to parse token: %s\n", tokenize);
        }

        tokenize = strtok(NULL, "\n"); 
    }

    int proc = process_in_the_list((unsigned int)pid, pid_array, i);


    if(proc == 0) {
        close(fd);
        return 0;
    }

    close(fd);
    return 1;


}

void register_task(unsigned int pid, unsigned long period, unsigned long computation) {

    char buffer[SIZE];
    int fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return;
    }

    // Format the registration string
    memset(buffer, 0, SIZE); // Clear the buffer
    sprintf(buffer, "R,%u,%lu,%lu", pid, period, computation);

    //printf("Buffer val: %s\n", buffer);
    // Write the string to /proc/mp2/status
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write to /proc/mp2/status");
        close(fd);
        return;
    }

    close(fd);
    
}

void yield(unsigned int pid) {
    
    char ybuf[SIZE];
    int fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return;
    }

    memset(ybuf, 0, SIZE); 
    sprintf(ybuf, "Y,%u\n",pid);

    write(fd, ybuf, strlen(ybuf));

    close(fd);

}

void deregister(unsigned int pid) {

    char dbuf[SIZE];
    int fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return;
    }
    memset(dbuf, 0, SIZE); 
    sprintf(dbuf, "D,%u\n",pid);

    //printf("Dbuf %s\n", dbuf);

    write(fd, dbuf, strlen(dbuf));
    close(fd);
}

void do_job(int i) {

    while(i--) {
    volatile long long unsigned int sum = 0;
    for (int i = 0; i < 1000; i++) {
        volatile long long unsigned int fac = 1;
        for (int j = 1; j <= 10; j++) {
            fac *= j;
        }
        sum += fac;
    }
    }

}


int process_in_the_list(unsigned int pid, int pid_arr[], int size) {
    for(int i = 0; i<size; i++) {
        if(pid_arr[i] == pid) {
            //printf("Process in the list helper, pid: %d\n", pid);
            return 1;
        }   

        else {
            continue;
        }  
    }
    return 0;
}


unsigned long get_time_ms(struct timespec time) {
     return time.tv_sec * 1000000000L + time.tv_nsec;
}