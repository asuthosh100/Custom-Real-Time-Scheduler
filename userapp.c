#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define FILE_PATH "/proc/mp2/status"
#define SIZE 2048

int main(int argc, char *argv[])
{
    pid_t pid = getpid(); 
    int fd; 
    char buffer[SIZE];

    unsigned int period = atoi(argv[1]); 
    unsigned int processing_time = atoi(argv[2]); 
//-------------REGISTER-------------------------------
    FILE *file = fopen(FILE_PATH, "w"); 
    
    if(file == NULL) {
        perror("Failed to open /proc");
        exit(1);
    }
    
    //fprintf(file, "R,%d,%u,%u\n", pid, period, processing_time);

    snprintf(buffer, sizeof(buffer), "R,%d,%lu,%lu\n", pid, period, processing_time);
    fwrite(buffer, sizeof(char), strlen(buffer), file);
   // printk("Sent to kernel "R,%d,%lu,%lu\n", pid, period, processing_time);
    printf("Sent to kernel:R,%d,%lu,%lu\n", pid, period, processing_time);
    fclose(file); 

    return 0;

}

