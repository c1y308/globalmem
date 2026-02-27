#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
void main(){
    int fd;
    int counter = 0;
    int old_counter = 0;

    fd = open("/dev/second", O_RDONLY);
    if(fd != -1){
        while(1){
            read(fd, &counter, sizeof(counter));
            if(counter != old_counter){
                printf("counter = %d\n", counter);
                old_counter = counter;
            }
        }
    }else{
        printf("open /dev/second failed\n");
    }
}