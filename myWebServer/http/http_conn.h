#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <mysql/mysql.h>
#include <fstream>

#include "../lock/locker.h"
#include "../CGI/mmysql/sql_conn_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has had syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

class http_conn {
public:
    // 文件名最大长度
    static const int FILENAME_LEN = 200;
    // 读缓冲区大小
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;
    // HTTP请求方法，但仅支持GET和POST
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH};
    // 主状态机状态
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
    // 服务器处理HTTP后可能的结果
    enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST,
                    FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
    // 从状态机，行读取状态
    enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};
public:
    http_conn() {};
    ~http_conn() {};
public:
    // 初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr, char*, int, int, string user, string passwd, string sqlname);
    // 关闭连接
    void close_conn(bool real_close = true);
    // 处理客户请求
    void process();
    // TODO:非阻塞读（reactor模式用）
    bool read_once();
    // TODO:非阻塞写（reactor模式用）
    bool write();

    sockaddr_in* get_aaddress() { return &m_address; }
    // TODO:以下三行作用分别是什么
    void initmysql_result(connection_pool* connPool);
    int timer_flag;
    int improv;
private:
    void init();
    // 分析HTTP请求
    HTTP_CODE process_read();
    // 填充HTTP应答
    bool process_write(HTTP_CODE ret);

    // 这一系列函数由process_read调用，分析HTTP请求
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line() {return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();

    // 这一系列函数由process_write调用，填充HTTP应答
    void unmap();
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
public:
    // 所有socket上的事件注册到同一个epollfd内核时间表中，所以将epollfd设置为静态的
    static int m_epollfd;
    // 统计用户数量
    static int m_user_count;
    // 将从数据库连接池中取一个连接
    MYSQL* mysql;
    // 涉及proactor下的数据io。
    // 0--客户数据到来，把数据读到读缓冲区，等待工作线程处理
    // 1--工作线程处理完毕，把数据从写缓冲区写到socket，发送给客户
    int m_state;
private:
    // 该HTTP的连接socket
    int m_sockfd;
    // 对方socket地址
    sockaddr_in m_address;

    // 读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    long m_read_idx; // 已读入客户端最后一个字符的下一个位置
    long m_checked_idx; // 正在分析字符在缓冲区的位置
    int m_start_line; // 正在解析行的起始位置
    // 写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx; // 待发送字节数

    CHECK_STATE m_check_state; // 主状态机状态
    METHOD m_method; // 请求方法

    // 请求目标完整路径 = doc_root + m_url
    char m_real_file[FILENAME_LEN];
    char* m_url;
    char* m_version; // http版本号
    char* m_host; // 主机名
    long m_content_length; // 请求消息体长度
    bool m_linger; // http请求是否要求保持连接

    // 客户请求目标文件被mmap到内存中的起始位置
    char* m_file_address; 
    // 目标文件状态，包含是否存在、是否为目录、是否可读、文件大小等
    struct stat m_file_stat; 
    // 
    struct iovec m_iv[2];
    // 被写内存块数量
    int m_iv_count;

    int cgi; // 是否启用POST
    char* m_string; // 存储请求消息体
    int bytes_to_send;
    int bytes_have_send;
    char* doc_root;

    // TODO: 实际上没用？
    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif