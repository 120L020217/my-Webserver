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
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

/*修改文件描述符的状态为非阻塞*/
int setnoblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/*将文件描述符fd上的EPOLLIN(数据可读)注册到epollfd指示的epoll内核时间表中，
参数enable_et指示是否开启ET模式*/
void addfd(int epollfd, int fd, bool enable_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_et) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // setnoblocking(fd);
} 

// LT
void lt(epoll_event* events, int number, int epollfd, int listenfd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; i++) {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            printf("test\n");
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*) &client_address, &client_addrlength);
            addfd(epollfd, connfd, false);
        } else if (events[i].events & EPOLLIN) {
            /*只要socket读缓存中还有未读出的数据，这段代码就被触发*/
            printf("event trigger once\n");
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0) {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        } else {
            printf("something else happened\n");
        }
    }
}

// ET
void et(epoll_event* events, int number, int epollfd, int listenfd) {
    printf("et\n");
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; i++) {
        printf("the %d turn of proceeding data\n", i);
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            printf("accept a new connection\n");
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*) &client_address, &client_addrlength);
            addfd(epollfd, connfd, true);
        } else if (events[i].events & EPOLLIN) {
            /* 不会被触发，循环读数据，是数据全部读出*/
            printf("event trigger once\n");
            while (1) {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if ( errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read later\n");
                        break;
                    }
                    close(sockfd);
                    break;
                } else if (ret == 0) {
                    close(sockfd);
                } else {
                    printf("get %d bytes of content: %s\n", ret, buf);
                }
            }
        } else {
            printf("something else happened\n");
        }
    }
}

int main (int argc, char * argv[]) {
    printf("%d, %s, %s, %s\n", argc, argv[0], argv[1], argv[2]);
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in  address;
    bzero( &address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret!=-1);
    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while (1) {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0) {
            printf("epoll failure\n");
            break;
        }
        // lt(events, ret, epollfd, listenfd);
        et(events, ret, epollfd, listenfd);
    }

    close(listenfd);
    return 0;    
}