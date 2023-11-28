#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
static const char* status_line[2] = {"200 OK", "500 Internal server error"};
int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // const char* file_name = argv[3];

    struct sockaddr_in  address;
    bzero( &address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret!=-1);

    /**
     * @brief 第二个参数backlog，内核监听队列中ESTABLISHED状态的socket的最大长度，如果超过backlog，
     * 服务器将不接受新的连接（客户的connect函数），客户端也受到ECONNREFUSED错误信息。
     * 但实际运行中，最大长度往往比backlog指示的值略大
     * 内核监听队列中，处于SYN_RCVD状态的最大数量由/proc/sys/net/ipv4/tcp_max_syn_backlog内核参数定义
     */
    ret = listen(sock, 5);
    assert(ret != -1);
    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof (client);
    /**
     * @brief 从监听队列中取一个ESTABLISHED，接受一个连接，并建立一个新的socket用来和他通信
     * 
     */
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0) {
        printf( "errno is: %d\n", errno);
    }
    else {
        close(STDOUT_FILENO);
        dup(connfd);
        printf("abcd\n");
        close(connfd);
    }

    close(sock);
    return 0;
    
}