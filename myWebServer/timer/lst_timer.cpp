#include "lst_timer.h"

sort_timer_lst::~sort_timer_lst() {
    util_timer* tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    if (!head) {
        head = tail = timer;
        return;
    }
    /* 如果timer的超时时间小于当前链表中所有定时器的超时时间，则把该定时器
    插入链表头部，否则调用重载函数add_timer插入合适的位置，保证链表升序*/
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    util_timer* tmp = timer->next;
    /*如果timer在当前链表尾部，则不用调整
    如果timer调整后仍小于后一个timer的超时时间，则不用调整*/
    if (!tmp || timer->expire < tmp->expire) { // 注意（!tmp）写在前
        return;
    }
    if (head == timer) {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL; // 可省
        add_timer(timer, head);
    } else {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, head);
    }
}

void sort_timer_lst::del_timer(util_timer *timer) {
    if (!timer) {
        return;
    }
    // 链表中只有一个节点
    if (timer == head && timer == tail) {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head) {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail) {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick() {
    if (!head) {
        return;
    }
    printf("timer tick\n");
    time_t cur = time(NULL); // 获取当前系统时间
    util_timer* tmp = head;
    /*（核心逻辑）从头结点开始依次处理每个定时器，直到遇到一个尚未到期的定时器*/
    while(tmp) {
        if (cur < tmp->expire) {
            break;
        }
        // 回调函数执行定时任务
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head) {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head) {
    util_timer *prev = lst_head;
    util_timer *tmp = timer->next;

    while (tmp) {
        if (tmp->expire > timer->expire) {
            timer->prev = prev;
            timer->next = tmp;
            prev->next = timer;
            tmp->prev = timer;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    if (!tmp) { // 遍历后仍没找到位置，说明应该插在队尾
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Utils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL); // 文件描述符旧的状态标志
    int new_option = old_option | O_NONBLOCK; // 非阻塞
    fcntl(fd, F_SETFL, new_option); // 设置文件描述符的状态标志
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, bool TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }  else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Utils::sig_handler(int sig) {

}

void Utils::addsig(int fd) {

}

void Utils::timer_handler() {

}

// TODO: 这两个类中定义的静态成员变量在类外定义和初始化
int *Utils::u_pipefd = nullptr;
int Utils::u_epollfd = 0;
