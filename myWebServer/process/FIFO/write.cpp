#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/**
 * @brief 只读模式打开一个管道，阻塞直到另一个进程只写模式打开管道
 * 
 * @return int 
 */

int main() {
    int ret = access("test", F_OK);
    if (ret == -1) {
        printf("管道不存在，创建管道\n");
        ret = mkfifo("test", 0664);
        if (ret == -1) {
            perror("mkfifo");
            exit(0);
        }
    }
    
    int fd = open("test", O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(0);
    }

    for (int i = 0; i < 100; i++) {
        char buf[1024];
        sprintf(buf, "hello, %d\n", i);
        printf("write data: %s\n", buf);
        write(fd, buf, strlen(buf));
        sleep(1);
    }
    close (fd);
    return 0;
}