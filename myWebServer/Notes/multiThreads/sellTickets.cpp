#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
int tickets = 100;

void * sellTicket(void *arg) {
    while (tickets > 0) {
        usleep(6000);
        printf("%d 正在卖第 %d 张门票\n", pthread_self(), tickets);
        tickets--;
    }
    return NULL;
}

int main () {
    pthread_t tid1, tid2, tid3;
    pthread_create(&tid1, NULL, sellTicket, NULL);
    pthread_create(&tid2, NULL, sellTicket, NULL);
    pthread_create(&tid3, NULL, sellTicket, NULL);


    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    pthread_exit(NULL); // 主线程退出，防止执行return 0，进程退出，子线程就都不能执行了

    return 0;

}