#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

class process {
public:
    process() : m_pid(-1) {}
public:
    pid_t m_pid; // 子进程id
    int m_pipefd[2]; // 父子进程通信管道
};

template<typename T>
class processpool{
private:
    processpool(int listenfd, int process_number = 8);
public:
    static processpool<T>* create(int listenfd, int process_number = 8) {
        if (!m_instance) {
            m_instance = new processpool<T>(listenfd, process_number);
        }
        return m_instance;
    }
    ~processpool() {
        delete[] m_sub_process; // 释放数组内存
    }
    void run(); // 启动线程池

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    static const int MAX_PROCESS_NUMBER = 16; // 进程池最大子进程数量
    static const int USER_PER_PROCESS = 65536; // 每个子进程处理最多的客户数量
    static const int MAX_EVENT_NUMBER = 10000; // epoll最多处理的事件数

    int m_process_number; // 进程池中进程总数
    int m_idx; // 子进程在池中的序号，从0开始
    int m_epollfd; // 每个进程在内核中都有一个epoll事件表，m_epollfd标识
    int m_listenfd;
    int m_stop; // 子进程通过m_stop决定是否停止运行

    process* m_sub_process; // 指向子进程

    static processpool<T>* m_instance; // 单例
};
template<typename T>
processpool<T>* processpool<T>::m_instance = nullptr;

static int sig_pipefd[2]; // 管道，用于统一事件源

static int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

static void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void sig_handler(int sig) {
    int save_errno = errno; // 保证可重入性，防止信号处理程序改变errno，从而影响主程序
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void (handler)(int), bool restart = true) { // 传递一个函数指针
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) { // 重启被中断的系统调用
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// TODO: 多进程和线程池类的关系:每个进程都有一个指向进程池对象的指针，这些进程池对象是相互对立的	
// 监听socket listenfd需要在创建进程池之前创建好
template<typename T>
processpool<T>::processpool(int listenfd, int process_number) : 
    m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));

    m_sub_process = new process[process_number];
    assert(m_sub_process);

    for (int i = 0; i < process_number; i++) {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if (m_sub_process[i].m_pid > 0) { // 父进程
            close(m_sub_process[i].m_pipefd[1]); // 关闭一端
            continue;
        } else { // 子进程
            close(m_sub_process[i].m_pipefd[0]); // 关闭一端
            m_idx = i;
            break;
        }
    }
}

// TODO: 管道写端设置为非阻塞，读端在addfd中也设为非阻塞
// sig_pipefd是一个全局静态的管道
template<typename T>
void processpool<T>::setup_sig_pipe() {
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);
    
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, sig_handler);
}

// 父进程idx为-1， 子进程设置为i（在函数create中）
template<typename T>
void processpool<T>::run() {
    if (m_idx != -1) {
        run_child();
        return;
    }
    run_parent();
}

template<typename T>
void processpool<T>::run_child() {
    // TODO: 用处：开一个epoll模型，设置好信号管道，加入epoll
    setup_sig_pipe();  

    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    // TODO: 子进程找到与父进程连接的管道，加入epoll
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T* users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;

    while (!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == pipefd && events[i].events & EPOLLIN) { // 子进程与父进程通信管道有数据可读，有客户连接请求
                int client = 0;
                ret = recv(sockfd, (char*)&client, sizeof(client), 0);
                if (( ret < 0 && errno != EAGAIN ) || ret == 0 ) { // 
                    continue;
                } else { // 有新的客户连接要处理
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof (client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr*) &client_address, $client_addrlength);
                    if (connfd < 0 ) {
                        printf("errno is: %d\n, errno");
                        continue;
                    }
                    addfd(m_epollfd, connfd);

                    // TODO: 模板类必须实现init方法，初始化一个客户链接？
                    // 我们直接用connfd索引逻辑处理对象（即T对象），提高程序效率
                    // 每个子进程可以处理多个用户，用users数组表示处理的用户集合，用connfd表示不同用户
                    users[connfd].init(m_epollfd, connfd, client_address);
                }
            } else if (sockfd == sig_pipefd[0] && events[i].events & EPOLLIN) { // 信号管道有数据可读（有信号要处理）
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) continue;
                else {
                    for (int i = 0; i < ret; i++) {
                        switch(signals[i]) {
                            case SIGCHLD:
                            { // 工作进程收到他的子进程发来的信号，及时处理他的子进程状态变化
                                pid_t pid;
                                int stat;
                                while (pid = waitpid(-1, &stat, WNOHANG) > 0)
                                    continue;
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                m_stop = true;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) { // 客户有数据到来，处理客户请求
                users[sockfd].process();
            } else {
                continue;
            }
        }
    }

    delete[] users;
    users = nullptr;
    // TODO: 还有sig_pipe，connfd 没有被销毁
    // connfd在removefd函数中被销毁
    close(pipefd);
    // close(m_listenfd); /* 应有创建listenfd的创建者来销毁*/
    close(m_epollfd);
}

template<typename T>
void processpool<T>::run_parent() {
    setup_sig_pipe();

    // 父进程监听m_listenfd
    adddfd(m_epollfd, m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0;
    int ret = -1;

    while(!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i <number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listerfd) { // 有新连接到来
                int i = sub_process_counter;
                do{
                    if (m_sub_process[i].m_pid != -1) { // 如果子进程没有被关闭
                        break;
                    }
                    i = (i+1) % m_process_number;
                } while(i != sub_process_counter);

                if (m_sub_process[i].m_pid == -1) { // 进程池中所有子进程都被关闭
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i+1) % m_process_number;

                send(m_sub_process[i].m_pipefd[0], (char*)&new_conn, sizeof(new_conn), 0);
                printf("send request to child %d\n", i);
            } else if (sockfd == sig_pipefd[0] && events[i].events & EPOLLIN) {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; i++) {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            /*非阻塞的waitpid，如果没有子进程状态变化，返回0，这样父进程不会阻塞在这个while循环中*/
                            while (pid = waitpid(-1, &stat, WNOHANG) > 0) { // 回收进程池中的一个子进程
                                for (int i = 0; i < m_process_number; ++i) {
                                    if (m_sub_process[i].m_pid == pid) {
                                        printf("chlid %d join\n", i);
                                        close(m_sub_process[i].m_pipefd[0]); // 关闭父进程与此子进程的通信管道
                                        m_sub_process[i].m_pid = -1;
                                    }
                                }
                            }
                            m_stop = true;
                            for (int i = 0; i < m_process_number; i++) {
                                if (m_sub_process[i].m_pid != -1) {
                                    m_stop = false;
                                }
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        { // 杀死所有子进程
                            for (int i = 0; i < m_process_number; i++) {
                                int pid = m_sub_process[i].m_pid;
                                if (pid != -1) {
                                    kill(pid, SIGTERM);
                                }
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            } else {
                continue;
            }
        }
    }

    close(m_epollfd);
}

#endif