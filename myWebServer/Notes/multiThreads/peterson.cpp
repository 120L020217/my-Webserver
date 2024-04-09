#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
using namespace std;

// 共享资源
int num = 1000;

// 全局变量
bool flag[2] = {false, false};
int turn = 0;  // 轮到哪个线程进入临界区

// 进入临界区的线程 i
void * iThread(void *arg) {
    pthread_t tid = pthread_self();
    while (1) {
        flag[0] = true;
        turn = 1; // 将控制权让给另一个线程
        while (flag[1] && turn == 1) {} // 等待另一个线程退出临界区
        
        // 临界区代码
        if (num > 0) {
            usleep(6000);
            printf("%d 正在卖第 %d 张门票\n", pthread_self(), num);
            num--;
        }
        
        // printf("num: %d, tid: %ld\n", num, tid);
        // cout << "num: " << num << ", tid: " << tid << endl;

        flag[0] = false;
        // sleep(1);
        // 退出临界区
    }
    
    return NULL;
}


// 进入临界区的线程 j
void * jThread(void * arg) {
    pthread_t tid = pthread_self();
    while (1) {
        flag[1] = true; // 1线程想进入临界区
        turn = 0; // 将控制权让给另一个线程
        while (flag[0] && turn == 0) {} // 等待另一个线程退出临界区
        
        // 临界区代码
        if (num > 0) {
            usleep(6000);
            printf("%d 正在卖第 %d 张门票\n", pthread_self(), num);
            num--;
        }

        // printf("num: %d, tid: %ld\n", num, tid);
        // cout << "num: " << num << ", tid: " << tid << endl;

        flag[1] = false;
        // sleep(1);
        // 退出临界区
    }
    
    return NULL;
}

int main() {
    pthread_t tidi, tidj;
    pthread_create(&tidi, NULL, iThread, NULL);
    pthread_create(&tidj, NULL, jThread, NULL);
    
    pthread_detach(tidi);
    pthread_detach(tidj);

    pthread_exit(NULL);

    return 0;

}

