#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class log {
public:
    static log* get_instance() {
        static log instance;
        return &instance;
    }

    // 异步日志的回调函数
    static void *flush_log_thread(void *arg) {
        log::get_instance()->async_write_log();
    }

    // 初始化日志，日志系统的启动函数
    // 创建(若已存在则打开)日志文件，根据同步、异步方式，设置参数
    // file_name:日志文件名
    // close_log:日志系统开关，0--开，1--关
    // log_buf_size:缓冲区大小
    // split_lines:日志文件最大行数
    bool init(const char* file_name, int close_log,int log_buf_size, int split_lines, int max_queue_size);
    // 写入文件（日志按时间分文件，文件内区分不同级别的日志，格式化输出）
    void write_log(int level, const char* format, ...);
    // 刷缓存，持久化日志
    void flush();

private:
    log() : m_count(0), m_is_async(false) {}
    virtual ~log() {
        if (m_fp != NULL) {
            fclose(m_fp);
        }
    }

    void* async_write_log() {
        string single_log;
        while (m_log_queue->pop(single_log)) {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    // 多线程访问这个单例中的共享资源，如m_fp，只有一个线程能控制文件描述符。
    // m_count, m_buf, 同时也被保护
    locker m_mutex;

    // 关闭日志的标志变量
    int m_close_log;

    // 当天已写行数   
    long long m_count;
    // 单文件最大行数
    int m_split_lines;

    char dir_name[128]; //日志路径名
    char log_name[128]; // 日志文件名
    FILE* m_fp; // 文件指针

    // 用户区的写缓冲
    char* m_buf;
    int m_buf_size; // 写缓冲的大小

    // 当前时间
    int m_today;

    // 异步相关
    bool m_is_async;
    block_queue<string> *m_log_queue;
    
};

#define LOG_DEBUG(format, ...) if (0 == m_close_log) { log::get_instance()->write_log(0, format, ##__VA_ARGS__); log::get_instance()->flush(); }
#define LOG_INFO(format, ...) if (0 == m_close_log) { log::get_instance()->write_log(1, format, ##__VA_ARGS__); log::get_instance()->flush(); }
#define LOG_WARN(format, ...) if (0 == m_close_log) { log::get_instance()->write_log(2, format, ##__VA_ARGS__); log::get_instance()->flush(); }
#define LOG_ERROR(format, ...) if (0 == m_close_log) { log::get_instance()->write_log(3, format, ##__VA_ARGS__); log::get_instance()->flush(); }


#endif