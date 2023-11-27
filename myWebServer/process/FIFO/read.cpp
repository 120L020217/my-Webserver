#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("test", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(0);
    }

    while (1)
    {
        char buf[1024] = {0};
        int len = read(fd, buf, sizeof(buf));
        if (len == 0) {
            printf("写段断开连接\n");
            break;
        }
        printf("%s\n", buf);
    }
    
}