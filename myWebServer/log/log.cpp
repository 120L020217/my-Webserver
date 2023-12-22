#include "log.h"

bool log::init(const char* file_name, int close_log,int buf_size, int split_lines, int max_queue_size) {
    // 异步设置，当max_queue_size为0，同步
    if (max_queue_size >= 1) {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        TODO:// "这个异步线程回收问题：由谁来回收?"
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_close_log = close_log;
    m_buf_size = buf_size;
    m_buf = new char[m_buf_size];
    memset(m_buf, '\0', m_buf_size);
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/'); // 返回路径末尾的文件名
    char log_full_name[256] = {0};
    if (p == NULL) { // 传进来的是相对路径
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else { // 传进来的是绝对路径
        strcpy(log_name, p+1);
        strncpy(dir_name, file_name, p-file_name+1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a"); // 追加写的方式打开
    if (m_fp == NULL) {
        return false;
    }

    return true;
}

/*
1. 确定写在哪个文件上
2. 确定日志的内容
*/
void log::write_log(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    
    char s[16] = {0};
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    m_mutex.lock();
    m_count++;

    // 写日志前检查日期是否正确，和是否达到最大行数，来控制打开的文件
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) { // 如果需要更换文件
        fflush(m_fp);
        fclose(m_fp);

        char new_log_full_name[256] = {0};
        char tail[16] = {0}; // 日志文件名前缀
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) { // 日期不正确
            snprintf(new_log_full_name, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 1;
        } else { // 达到最大行数
            snprintf(new_log_full_name, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }

        m_fp = fopen(new_log_full_name, "a");
    }

    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int m = snprintf(m_buf + n, m_buf_size - n - 1, format, valst);
    m_buf[n+m] = '\n';
    m_buf[n+m+1] = '\0';
    log_str = m_buf; 

    m_mutex.unlock();

    // 即使是异步日志，如果阻塞队列已经满了，仍采用同步方式，防止太多进程睡在阻塞队列上
    if (m_is_async && !m_log_queue->full()) {
        m_log_queue->push(log_str);
    } else {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void log::flush() {
    m_mutex.lock();
    fflush(m_fp); // 确保数据从用户空间流的缓冲区写到内核缓冲区
    m_mutex.unlock();
}