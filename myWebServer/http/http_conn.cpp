#include "utility.h"
#include "http_conn.h"

// TODO: 用处?
locker m_lock;
map<string, string> users;


int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool real_close) {
    if (real_close && m_sockfd != -1) {
        LOG_INFO("close %d\n", m_sockfd);
        // printf("close %d\n", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void http_conn::init(int sockfd, const sockaddr_in &addr, char* root, int TRIGMode,
                     int close_log, string user, string passwd, string sqlname) {
    m_sockfd = sockfd;
    m_address = addr;
    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());

    addfd(m_epollfd, sockfd, true, m_TRIGMode);
    m_user_count++;

    init();
}

void http_conn::init() {
    timer_flag = 0;
    improv = 0;
    mysql = NULL;
    m_state = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    m_read_idx = 0;
    m_checked_idx = 0;
    m_start_line = 0;
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    m_write_idx = 0;

    m_check_state = CHECK_STATE_REQUESTLINE;
    m_method = GET;

    memset(m_real_file, '\0', FILENAME_LEN);
    m_url = 0;
    m_version = 0;
    m_host = 0;
    m_content_length = 0; 
    m_linger = false;

    cgi = 0;
    bytes_to_send = 0;
    bytes_have_send = 0;
}

http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for ( ; m_checked_idx < m_read_idx; ++m_checked_idx) {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r') {
            if (m_checked_idx + 1 == m_read_idx) 
                return LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] = '\n') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r') {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
        }
    }
    return LINE_OPEN;
}

// TODO: 这个函数返回值代表什么，如果是是否正确接收数据，那么客户发送完数据关闭连接返回false，但也是一次正确的接收数据呀
bool http_conn::read_once() {
    if (m_read_idx >= READ_BUFFER_SIZE) 
        return false;
    int bytes_read = 0;

    if (0 == m_TRIGMode) { // LT
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;
        
        if (bytes_read <= 0) return false;
        return true;
    } else { // ET
        while (true) {
           bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1) { 
                if (errno = EAGAIN || errno == EWOULDBLOCK) break; // 连接未关闭，没有数据可读
                return false; // 连接出现问题，接受数据失败
            } else if (bytes_read == 0) return false; // 连接已关闭
            m_read_idx += bytes_read;
        }
        return true;
    }
}

http_conn::HTTP_CODE http_conn::parse_request_line(char* text) {
    m_url = strpbrk(text, " \t");
    if (!m_url) return BAD_REQUEST;
    *m_url++ = '\0';
    char* method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
        // 既然只是标记POST，这个开关是否有意义，m_method就足够了
        cgi = 1;
    } else {
        return BAD_REQUEST;
    }
    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version) return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0) 
        return BAD_REQUEST; // 支持http1.1
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }

    if (strlen(m_url) == 1) // url为单独的 / ，显示判断界面
        strcat(m_url, "judge.html");
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST; // 请求不完整
}

http_conn::HTTP_CODE http_conn::parse_headers(char* text) {
    if (text[0] == '\0') { // 遇到空行，说明头部解析完
        if (m_content_length != 0) { // 如果有消息体，还需读取m_content_length字节的消息体
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) 
            m_linger = true;
    } else if (strncasecmp(text, "Content-length", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        LOG_INFO("oop! unknow header %s\n", text);
    }
}

// 并没有解析消息体，只是判断是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char* text) {
    if (m_read_idx >= (m_content_length + m_checked_idx)) {
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
}