#include "threadpool.h"

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool* connPool, int thread_number, int max_requests)
    : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(nullptr), m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0) 
        throw std::exception();
    m_thread = new pthread_t[m_thread_number];
    if (!m_threads) 
        throw std::exception();
    for (int i = 0; i < thread_number; ++i) {
        if (pthread_create(m_thread + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        // 设置为脱离线程，线程终止，自动释放资源
        if (pthread_detach(m_thread[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T* request, int state) {
    m_queuelocker.lock();

    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);

    m_queuelocker.unlock();
    m_queuelocker.post();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T* request) {
    m_queuelocker.lock();

    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    
    m_queuelocker.unlock();
    m_queuelocker.post();
    return true;
}

template <typename T>
void* threadpool<T>::worker(void* arg) {
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run() {
    while (true)
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlocker();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        if (1 == m_actor_model) { // proactor
            if (0 == request->state) { // 读请求
                if (request->read_once()) {
                    request->improv = 1;
                    // 取一个数据库连接
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                } else {
                    // TODO: 这两个参数有什么价值
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else { // 写请求
                if (request->write()) {
                    // TODO: 为什么不需要处理request->process()?
                    request->improv = 1;
                } else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else { // reactor: 直接交给request类处理, 包括数据的读写
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            // TODO: request如何判断是否已经完成数据读写
            request->process();
        }
    }
    
}