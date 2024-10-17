#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define FILE_PATH "/proc/mp2/status"
#define SIZE 4096

int main(int argc, char *argv[])
{
    pid_t pid = getpid(); 
    int fd; 
    char buffer[SIZE];

    unsigned int period = atoi(argv[1]); 
    unsigned int processing_time = atoi(argv[2]); 
//-------------REGISTER-------------------------------
    // FILE *file = fopen(FILE_PATH, "w"); 
    
 

    fd = open(FILE_PATH, O_WRONLY); 
    if(fd == -1) {
        perror("Failed to open /proc");
        return 1;
    }
    
    //fprintf(file, "R,%d,%u,%u\n", pid, period, processing_time);

    snprintf(buffer, sizeof(buffer), "R,%d,%u,%u\n", pid, period, processing_time);

    printf("Buffer val : %s\n", buffer);
    // size_t bytes_written = fwrite(buffer, sizeof(char), strlen(buffer), file);

    // if (bytes_written != strlen(buffer)) {
    //     printf("Failed to write the entire buffer to /proc\n");
    // }

    if(write(fd, buffer, strlen(buffer))<0) {
        perror("Failed to Write to Kernel Module");
        close(fd);
        return -1;
    }
    
    printf("Sent to kernel:R,%d,%u,%u\n", pid, period, processing_time);
    close(fd); 

    return 0;

}

