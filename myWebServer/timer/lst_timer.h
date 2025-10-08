#ifndef LST_TIMER
#define LST_TIMER
/* 升序定时器链表 */
#include <time.h>
#include <stdio.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "../log/log.h"

#define BUFFER_SIZE 64
class util_timer; /*前向声明*/

// 用户数据结构：客户端socket地址、socket文件描述符、读缓存、计时器
struct client_data { 
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

class util_timer {
public:
    util_timer () : prev(NULL), next(NULL) {}

public:
    time_t expire; /*任务超时时间（绝对时间）*/
    void (*cb_func)(client_data*); /*任务回调函数*/ 
    client_data* user_data; /*回调函数处理的客户数据，由定时器的执行者传递给回调函数*/
    util_timer* prev; /* 指向前一个计时器 */
    util_timer* next; /* 指向后一个计时器 */
};

class sort_timer_lst {
public:
    sort_timer_lst() : head(NULL), tail(NULL) {}
    ~sort_timer_lst(); // 析构，删除所有定时器

    void add_timer(util_timer *timer); // 链表中添加一个定时器
    /* 当某个定时任务发生变化时，调整定时器在链表中的位置。
    只考虑被调整的定时器超时延长的情况，即定时器需要向链表尾部移动 */
    void adjust_timer(util_timer *timer); 
    void del_timer(util_timer *timer); // 链表中删除一个定时器
    /* SIGALRM信号每次被触发就在其信号处理函数（如果使用统一事件源，则是主函数）
    中执行一次tick函数，已处理链表上的到期任务*/
    void tick();

private:
    /* 重载add_timer函数，被public的add_timer和adjust_timer函数调用
    表示将目标定时器timer添加到节点lst_head之后的部分链表中
    （不可能插到头节点）（链表中节点数大于1） */
    void add_timer(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};

class Utils {
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    // 对文件描述符设置非阻塞，否则线程阻塞在网络io函数send、recv上。
    int setnonblocking(int fd);
    
    // 注册epoll事件，
    // one_shot:开启EPOLLONWESHOT，文件描述符上监听的事件只能被触发一次
    // 避免了因事件多次到来（默认ET模式下）不同工作线程依次响应，使得多个线程操作一个文件描述符
    // TRIGMode：et模式or lt模式，1是et模式。
    void addfd(int epollfd, int fd, bool one_shot, bool TRIGMode);

    //为什么是静态函数：因为要使用静态成员变量u_pipefd
    static void sig_handler(int sig);

    void addsig(int sig);

    void timer_handler();

    //TODO:showerror函数用在何处?

public:
    //TODO:为什么是静态变量?
    static int *u_pipefd;
    static int u_epollfd;
    sort_timer_lst m_timer_lst;
    int m_TIMESLOT; // 定时器定时时间，多久后发送信号
};

#endif