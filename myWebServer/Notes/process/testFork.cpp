#include <stdio.h>
#include <unistd.h>

int main(){
    pid_t pid = fork();
    FILE * fp = fopen("test1.txt", "r+");
    char buffer[] = "qwe";

    if (pid > 0) {
        printf("父进程, pid= %d, ppid= %d\n", getpid(), getppid());
        
        size_t written = fwrite(buffer, 1, sizeof(buffer), fp);
        printf("%d\n", written);
        if (written != sizeof(buffer)) {
            perror("Error writing to file");
        }
        fclose(fp);
        printf("父进程%s\n", buffer);
    } else if (pid == 0) {
        printf("子进程, pid= %d, ppid= %d\n", getpid(), getppid());
        sleep(1);
        size_t read = fread(buffer, 1, 4, fp);
        printf("%d\n", read);
        if (read != sizeof(buffer)) {
            perror("Error reading from file");
        }
        printf("子进程%s\n", buffer);
        fclose(fp);
    }

    // for (int i = 0; i < 3; i++) {
    //     printf("pid= %d, %d\n", getpid(), i);
    //     sleep(1);
    // }
    return 0;
}