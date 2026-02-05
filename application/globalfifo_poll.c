#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

int main(void)
{
    int fd, num;
    char buffer[BUFFER_LEN];
    
    struct pollfd fds[1];
    
    fd = open("/dev/globalfifo", O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        perror("open /dev/globalfifo failed");
        return 1;
    }
    
    if (ioctl(fd, FIFO_CLEAR, 0) < 0) {
        perror("ioctl FIFO_CLEAR failed");
        close(fd);
        return 1;
    }

    while (1) {
        //  设置 pollfd 结构体
        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLOUT;  // 同时监视可读和可写
        fds[0].revents = 0;

        //  poll(数组, 数组长度, 超时毫秒)
        num = poll(fds, 1, -1);
        if (num < 0) {
            perror("poll failed");
            break;
        }
        
        //  检查 revents，是否可读
        if (fds[0].revents & POLLIN) {
            memset(buffer, 0, BUFFER_LEN);
            num = read(fd, buffer, BUFFER_LEN);
            if (num < 0) {
                perror("read failed");
                break;
            }
            printf("read %d bytes: %s\n", num, buffer);
        }
        
        //  检查 revents，是否可写
        if (fds[0].revents & POLLOUT) {
            num = write(fd, buffer, BUFFER_LEN);
            if (num < 0) {
                perror("write failed");
                break;
            }
            printf("write %d bytes\n", num);
        }
    }
    
    close(fd);
    return 0;
}