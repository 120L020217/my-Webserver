#include <pthread.h>
 /* thread:创建的线程id保存在这个结构中
    attr:设置线程属性,一半为NULL
    start_routine:函数指针,指向线程执行流
    arg:上一个参数的传参

    return:成功返回0,失败返回errornum.这个错误号与errno不一样
    获取错误号信息:char* strerror(int errornum)*/
// int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
//                     void *(*start_routine) (void *), void *arg);
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void * callback(void* arg){
    printf("child thread...\n");
    return NULL; // 相当于 pthead_exit(NULL)
}
int main () {
    pthread_t tid;
    
    int ret = pthread_create(&tid, NULL, callback, NULL);

    if (ret != 0) {
        char *err = strerror(ret);
        printf("error: %s \n", err);
    }

    for (int i = 0; i < 3 ;i++) {
        printf("%d\n", i);
    }
    pthread_exit(NULL);
    return 0;
}