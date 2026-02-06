#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

void main(void){
    int fd;
    fd = open("/dev/globalfifo", O_RDWR | O_NONBLOCK);
    if(fd != -1){
        struct epoll_event ev_globalfifo;
        int err;
        if(ioctl(fd, FIFO_CLEAR, 0) == -1){
            perror("ioctl FIFO_CLEAR failed\n");
            return;
        }

        int epfd = epoll_create1(0);
        if(epfd == -1){
            perror("epoll_create1 failed\n");
            return;
        }

        bzero(&ev_globalfifo, sizeof(ev_globalfifo));
        ev_globalfifo.events = EPOLLIN | EPOLLPRI;
        
        err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_globalfifo);
        if(err == -1){
            perror("epoll_ctl failed\n");
            return;
        }
        err = epoll_wait(epfd, &ev_globalfifo, 1, 15000);
        if(err == -1){
            perror("epoll_wait failed\n");
            return;
        }else if(err == 0){
            printf("no data input in FIFO within 15s\n");
        }else{
            printf("FIFO is ready to read\n");
        }

        err = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev_globalfifo);
        if(err == -1){
            perror("epoll_ctl failed\n");
            return;
        }
    }else{
        perror("open /dev/globalfifo failed\n");
        return;
    }
}