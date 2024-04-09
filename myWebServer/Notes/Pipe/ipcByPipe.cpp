#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

int main () {
    int fd[2];
    int ret = pipe(fd);
    if (ret == -1) {
        perror("pipe");
    }

    pid_t pid = fork();

    if (pid > 0 ) {
        close(fd[1]);
        char buf[1024];
        int len;
        while ((len = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
            printf("%s", buf);
            memset(buf, 0, 1024);
        }
        printf("test");
        wait(NULL);
    } else if (pid == 0) {
        close(fd[0]);
        // 重定向文件描述符STDOUT_FILENO到fd[1]
        dup2(fd[1], STDOUT_FILENO);
        execlp("ps", "ps", "aux", NULL);
        perror("execlp");
        exit(0);
    } else {
        perror("fork");
    }
}