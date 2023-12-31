#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class block_queue {
public: 
    block_queue(int max_size);
    ~block_queue();

    void clear();
    bool full();
    bool empty();
    bool front(T& value);
    bool back(T& value);
    int size();
    int max_size();
    /*item 为压入项的引用*/
    bool push(const T& item);
    /*item 为弹出项的地址*/
    bool pop(T& item);
    /*ms_timeout 以ms为单位，定义超时时间，
    规定如果此时队列为空，则最多等待的时间*/
    bool pop(T& item, int ms_timeout);

private:
    /*这里的锁有两个作用：
    1. 保护条件变量
    2. 保护阻塞队列的私有变量*/
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_max_size;
    // 指向队首前一个位置
    int m_front;
    // 指向队尾
    int m_back;
};

#endif