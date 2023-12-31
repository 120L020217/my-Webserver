#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
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
    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof (client);
    int connfd = accept(listenfd, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf( "errno is: %d\n", errno);
    }
    
    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1) {
        memset(buf, '\0', sizeof(buf));
        /**
         * @brief 每次调用select前，重新在read_fds和exception_fds中设置connfd
         * 因为事件发生后，文件描述符将被内核修改
         * 
         */
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);
        ret = select(connfd+1, &read_fds, NULL, &exception_fds, NULL);
        if (ret < 0) {
            printf("selection failture\n");
            break;
        }

        // 可读事件，普通的recv读数据
        if (FD_ISSET(connfd, &read_fds)) {
            ret = recv(connfd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);
        } 
        // 异常事件，MSG_OOB的recv读数据
        else if (FD_ISSET(connfd, &exception_fds)) {
            ret = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);
        }

    }

    close(connfd);
    close(listenfd);
    return 0;
    
}