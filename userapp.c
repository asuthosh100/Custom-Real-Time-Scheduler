#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define FILE_PATH "/proc/mp2/status"
#define SIZE 100

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    char pid_str[10]; // Buffer to hold the string representation of the PID
    sprintf(pid_str, "%d", pid); // Convert the integer PID to a string

    printf("Successfully registered process with PID: %s\n", pid_str);

    int fd; 
    char buffer[SIZE];
    char rbuf[SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <period> <processing_time>\n", argv[0]);
        return 1;
    }

    char *period = argv[1];
    char *processing_time = argv[2];

   printf("Per: %s and Comp: %s\n", period, processing_time);

//-------------REGISTER-------------------------------

    //Open the file directly for writing
    fd = open(FILE_PATH, O_RDWR); 
    if(fd == -1) {
        perror("Failed to open /proc/mp2/status");
        return 1;
    }

    // Format the registration string
    memset(buffer, 0, SIZE); // Clear the buffer
    sprintf(buffer, "R,%s,%s,%s\n", pid_str, period, processing_time);

    printf("Buffer val: %s\n", buffer);
    // Write the string to /proc/mp2/status
    if (write(fd, buffer, strlen(buffer)) == -1) {
        perror("Failed to write to /proc/mp2/status");
        close(fd);
        return 1;
    }
   

    read(fd, rbuf, sizeof(rbuf));
    printf("Read Buffer val: %s\n", rbuf);
    puts(rbuf);

    // Close the file
    close(fd);

    //printf("Successfully registered process with PID: %s\n", pid_str);

    return 0;
}
