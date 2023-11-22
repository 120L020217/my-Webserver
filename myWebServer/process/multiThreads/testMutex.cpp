/**
 * @brief 互斥量类型 pthread_mutex_t
 * #include <pthread.h>

 *  int pthread_mutex_init(pthread_mutex_t *restrict mutex,
                const pthread_mutexattr_t *restrict attr);
    mutex:要初始化的互斥量
    attr：互斥量相关属性
    restrict修饰符：被修饰的指针指向的内容，不能被另一个指针操作。
                保证只有唯一指针能操作它指向的内容。
    int pthread_mutex_destroy(pthread_mutex_t *mutex);

    int pthread_mutex_lock(pthread_mutex_t *mutex);
    已经被锁则阻塞
    int pthread_mutex_trylock(pthread_mutex_t *mutex);
    尝试加锁，立即返回，即使被锁上也立即返回
    int pthread_mutex_unlock(pthread_mutex_t *mutex);
 * 
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
int tickets = 100;

pthread_mutex_t mutex;

void * sellTicket(void *arg) {

    while (1) {
        // 加锁
        pthread_mutex_lock(&mutex);

        if (tickets > 0) {
            usleep(6000);
            printf("%d 正在卖第 %d 张门票\n", pthread_self(), tickets);
            tickets--;
            // 解锁
            pthread_mutex_unlock(&mutex);
        } else {
            // 解锁
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    return NULL;
}

int main () {

    pthread_mutex_init(&mutex, NULL);

    pthread_t tid1, tid2, tid3;
    pthread_create(&tid1, NULL, sellTicket, NULL);
    pthread_create(&tid2, NULL, sellTicket, NULL);
    pthread_create(&tid3, NULL, sellTicket, NULL);

    // 阻塞函数，可以保证主线程一定晚于子线程结束
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    pthread_mutex_destroy(&mutex);

    pthread_exit(NULL); // 让主线程退出，防止执行return 0，进程退出，子线程就都不能执行了

    return 0;

}