#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 默认情况下，这两个文件描述符都是阻塞的，
 * 这意味着，read函数和write函数的行为都是阻塞的
 * 
 * 这个程序存在进程安全问题，涉及到共享资源（文件描述符、管道）的操作，
 * 缺乏同步机制，可能导致竞态条件
 * 但从实际程序运行结果来看，尽管存在风险，父子进程交替地很好，
 * 每当管道写入1k的数据，就会切走
 * @return int 
 */

int main () {
    // fork前创建管道，否则无法使用匿名管道
    int pipefd[2];
    int ret = pipe(pipefd);
    if (ret == -1) {
        perror("pipe");
        exit(0);
    }

    pid_t pid = fork();
    if (pid > 0) {
        printf("i am parent process, pid= %d\n", getpid());
        // 关闭写端
        close(pipefd[1]);
        char buf[1024] = {'\0'};
        while(1) {
            int len = read(pipefd[0], buf, sizeof(buf));
            printf("parent rev: %s, pid: %d\n", buf, getpid());

            // const char * str = "hello i am parent";
            // write(pipefd[1], str, strlen(str));
            // sleep(1);
        }
        
    } else if (pid == 0) {
        printf("i am child process, pid= %d\n", getpid());
        // 关闭读端
        close(pipefd[0]);
        char buf[1024] = {'\0'};
        while (1) {
            const char * str = "hello i am child";
            write(pipefd[1], str, strlen(str));
            // sleep(1);

            // int len = read(pipefd[0], buf, sizeof(buf));
            // printf("chlid rev: %s, pid: %d\n", buf, getpid());
        }
        
    }
    return 0;
}