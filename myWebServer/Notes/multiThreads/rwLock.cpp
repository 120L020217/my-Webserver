/**
 * pthread_rwlock_t
 * int pthread_rwlock_init(pthread_rwlock_t * restrict rwlock, const pthread_rwlockattr_t *restrict attr);
 * int pthread_rwlock_destory(pthread_rwlock_t *rwlock);
 * int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
 * int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
 * int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
 * int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
 * int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
 * 
 * 8个线程操作一个全局变量, 3个写，5个读
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int num = 1;
pthread_rwlock_t rwLock;

void * writeNum(void* arg) {
    while(1) {
        pthread_rwlock_wrlock(&rwLock);
        num++;
        printf("++write, tid: %ld, num: %d\n", pthread_self(), num);
        pthread_rwlock_unlock(&rwLock);
        usleep(100);
    }

    return NULL;
}

void *readNum(void* arg){
    while(1) {
        pthread_rwlock_rdlock(&rwLock);
        printf("==read, tid: %ld, num: %d\n", pthread_self(), num);
        pthread_rwlock_unlock(&rwLock);
        usleep(100);
    }

    return NULL;
}

int main (){
    pthread_rwlock_init(&rwLock, NULL);

    pthread_t wtid[3], rtid[5];

    for (int i = 0; i < 3; i++){
        pthread_create(&wtid[i], NULL, writeNum, NULL);
    }

    for (int i = 0; i < 5; i++){
        pthread_create(&rtid[i], NULL, readNum, NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_detach(wtid[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_detach(rtid[i]);
    }

    pthread_exit(NULL);
}