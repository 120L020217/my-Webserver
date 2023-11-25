#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main () {

    pid_t pid;
    for (int i = 0;  i < 5; i++) {
        pid = fork();
        if (pid == 0) 
        {
            break;
        }
    }

    if (pid > 0) {
        while (1) {
            printf("parent, pid = %d\n", getpid());

            int ret = wait(NULL);
            printf("child die, pid = %d\n", ret);

            sleep(1);
        }
        
    } else if (pid == 0) {
        while(1) {
            printf("child, tid = %d\n", getpid());
            sleep(1);
        }
        
    }

    return 0;
}