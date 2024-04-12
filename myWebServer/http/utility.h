#include "http_conn.h"

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// EPOLLONESHOT作用：一个socket只能被一个线程处理
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
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

void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 当一个线程处理完成后，重置EPOLLONESHOT事件，使得接下来的事件能被其他线程处理
// ev是注册的事件类型
void modfd(int epollfd, int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMode) 
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else 
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
