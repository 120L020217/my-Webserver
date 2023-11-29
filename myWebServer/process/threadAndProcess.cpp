#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
/**在线程中fork，得到的子进程是这个线程的副本，这个子进程是
 * 单线程的，不会copy其他线程。同时这个子进程继承父进程互斥量
 * （条件变量类似）的状态（锁上还是没锁上），且子进程无法得知他的状态。
 * 如果子进程继承的锁是锁上的，而且还是由父进程其他线程锁上的，
 * 会出现死锁。
*/
pthread_mutex_t mutex;

/*子线程运行函数，首先获得锁，然后暂停5s，然后释放锁*/
void* another(void *arg) {
    printf("in child thread, lock the mutex\n");
    pthread_mutex_lock(&mutex);
    sleep(5);
    pthread_mutex_unlock(&mutex);
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_t id;
    pthread_create(&id, NULL, another, NULL);
    /*父进程休眠1s，保证子进程获得锁并上锁*/
    sleep(1);
    int pid = fork();
    if (pid < 0) {
        pthread_join(id, NULL);
        pthread_mutex_destroy(&mutex);
        exit(1);
    }else if (pid == 0) {
        printf("I am in the child, want to get the lock\n");
        /*子进程继承mutex状态，锁住。所以子进程加锁会死锁
        因为没人会给子进程的锁解锁了，解锁的线程没有copy过来*/
        pthread_mutex_lock(&mutex);
        printf("I can not run to here, oop...\n");
        pthread_mutex_unlock(&mutex);
        exit(0);
    }else {
        wait(NULL);
    }
    pthread_join(id, NULL);
    pthread_mutex_destroy(&mutex);
    return 0;
}