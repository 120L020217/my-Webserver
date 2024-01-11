#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGI/mmysql/sql_conn_pool.h"

template<typename T>
class threadpool {
public:
    threadpool(int actor_model, connection_pool* connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);
    bool append_p(T* request);
private:
    // 为什么是静态函数：因为这个函数作为工作线程的入口函数
    // pthread_create()函数第三个参数要求传递一个函数指针，
    // 如果传递成员函数，会因为this指针导致类型不匹配
    static void* worker(void* arg);
    void run();
private:
    pthread_t *m_threads;
    int m_thread_number;

    std::list<T*> m_workqueue;
    int m_max_requests;    
    locker m_queuelocker;
    sem m_queuestat;
    // bool m_stop;
    connection_pool* m_connPool;
    int m_actor_model; // reactor proactor切换
};


#endif