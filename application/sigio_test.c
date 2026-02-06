#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>  // 新增：处理错误码
#include <string.h> // 新增：memset

#define MAX_LEN 100

// 信号处理函数：仅使用异步信号安全函数（write）
void input_handler(int num) {
    char data[MAX_LEN];
    ssize_t len; 
    
    // 清空缓冲区，避免脏数
    memset(data, 0, sizeof(data));
    
    // 读取标准输入，处理read的返回值
    len = read(STDIN_FILENO, data, MAX_LEN - 1); // 预留1个字节给'\0'，避免越界
    
    // 分情况处理read返回值
    if (len == -1){
        const char *err_msg = "read error!\n";
        write(STDOUT_FILENO, err_msg, strlen(err_msg));
        return;
    } else if (len == 0) {
        // 处理EOF（Ctrl+D）
        const char *eof_msg = "EOF received, exit!\n";
        write(STDOUT_FILENO, eof_msg, strlen(eof_msg));
        _exit(0); // 信号处理中退出用_exit（异步安全），而非exit
    }
    
    // 手动添加字符串结束符（确保不越界）
    data[len] = '\0';
    
    // 用write替代printf（异步信号安全）
    const char *prefix = "input: ";
    write(STDOUT_FILENO, prefix, strlen(prefix));
    write(STDOUT_FILENO, data, len);
}


int main(void) {
    int oflags;
    int ret; // 用于接收fcntl返回值
    
    // 处理signal函数的错误
    if (signal(SIGIO, input_handler) == SIG_ERR) {
        perror("signal set SIGIO failed");
        return 1;
    }
    
    // 设置标准输入的属主为当前进程（错误处理）
    ret = fcntl(STDIN_FILENO, F_SETOWN, getpid());
    if (ret == -1) {
        perror("fcntl F_SETOWN failed");
        return 1;
    }
    
    // 获取文件状态标志（错误处理）
    oflags = fcntl(STDIN_FILENO, F_GETFL);
    if (oflags == -1) {
        perror("fcntl F_GETFL failed");
        return 1;
    }
    
    // 设置FASYNC标志（错误处理）
    ret = fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC);
    if (ret == -1) {
        perror("fcntl F_SETFL failed");
        return 1;
    }
    
    // 替换忙等循环：用pause()休眠，等待信号（CPU占用率0%）
    while (1) {
        pause(); // 进程休眠，直到收到任意信号
    }
    
    return 0;
}