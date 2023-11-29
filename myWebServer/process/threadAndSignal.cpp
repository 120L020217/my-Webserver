#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
/**
 * @brief 多线程共享信号：信号到来时，一个进程的所有线程都会接收到这个信号
 * 本例中，设置一个线程专门捕捉共享信号，具体操作是：
 * 主线程和其他子线程屏蔽共享信号
 * 一个线程专门通过sigwait用来捕获共享信号
 * 
 */
#define handler_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE);} while (0)

static void* sig_thread(void *arg) {
    sigset_t *set = (sigset_t *)arg;
    int s, sig;
    for ( ; ; ) {
        s = sigwait(set, &sig);
        if (s != 0) handler_error_en(s, "sigwait");
        printf("Signal handling thread got signal %d\n", sig);
    }
} 
int main () {
    pthread_t thread;
    sigset_t set;
    int s;
    
    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGUSR1);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s != 0) handler_error_en(s, "pthread_sigmask");
    s = pthread_create(&thread, NULL, &sig_thread, (void *)&set);
    if (s != 0) handler_error_en(s, "pthread_create");

    pause();
}
    