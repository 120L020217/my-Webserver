#ifndef LST_TIMER
#define LST_TIMER
/* 升序定时器链表 */
#include <time.h>
#include <stdio.h>
// #include <sys/socket.h>
#include <netinet/in.h>
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


#endif