#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem {
public:
    sem() {
        if (sem_init(&m_sem, 0, 0) != 0) {
            /*构造函数没有返回值，通过抛出异常来报告错误*/
            throw std::exception();
        }
    }
    sem(int num) {
        if (sem_init(&m_sem, 0, num) !=  0) {
            throw std::exception();
        }
    }
    ~sem() {
        sem_destroy(&m_sem);
    }
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }
    bool post() {
        return sem_wait(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

class locker {
public:
    locker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }
    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    ///*block 阻塞队列模块：返回内部封装的锁，用于条件变量的wait函数*/
    pthread_mutex_t *get() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class cond {
public:
    cond() {
        // if (pthread_mutex_init(&m_mutex, NULL) != 0 ) {
        //     throw std::exception();
        // }
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            // pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond() {
        pthread_cond_destroy(&m_cond);
    }
    /*传入一个已经上好锁的参数 m_mutex*/
    bool wait(pthread_mutex_t *m_mutex) {
        int ret = 0; 
        // pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t) {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        
        return ret == 0;
    }
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
    // pthread_mutex_t m_mutex;
};

#endif