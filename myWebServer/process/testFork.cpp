#include <stdio.h>
#include <unistd.h>

int main(){
    pid_t pid = fork();
    FILE * fp = fopen("test1.txt", "r");
    char buffer[4] = {'\0'};

    if (pid > 0) {
        printf("父进程, pid= %d, ppid= %d\n", getpid(), getppid());
        fclose(fp);
        printf("父进程%s\n", buffer);
    } else if (pid == 0) {
        printf("子进程, pid= %d, ppid= %d\n", getpid(), getppid());
        sleep(1);
        fread(buffer, 1, 3, fp);
        printf("子进程%s\n", buffer);
        fclose(fp);
    }

    // for (int i = 0; i < 3; i++) {
    //     printf("pid= %d, %d\n", getpid(), i);
    //     sleep(1);
    // }
}