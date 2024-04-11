#ifndef INCLUDE_COMMON_HPP
#define INCLUDE_COMMON_HPP

#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <list>
#include <strings.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999
#define EPOLL_SIZE 5
#define BUF_SIZE 1024
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"
#define SERVER_MESSAGE "ClientID %d say >> %s"
#define CAUTION "There is only one int the char room!"
#define EXIT "exit"

extern std::list<int> clients_list;

inline int send_broadcast_message (int client_fd)
{
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero (buf, BUF_SIZE);
    bzero (message, BUF_SIZE);

    printf ("read from client [%d] \n", client_fd);
    // 从客户端读消息
    int len = recv (client_fd, buf, BUF_SIZE, 0);
    if (len <= 0)
    {
        close (client_fd);
        clients_list.remove(client_fd);
        printf ("client [%d] closed.\n", client_fd);
    }
    else
    {
        if (clients_list.size() == 1)
        {
            send (client_fd, CAUTION, strlen (CAUTION), 0);
            return len;
        }

        sprintf (message, SERVER_MESSAGE, client_fd, buf);

        for (auto &&client : clients_list)
        {
            // 给其他客户端发送
            if (client != client_fd)
            {
                if (send (client, message, BUF_SIZE, 0) < 0)
                {
                    perror ("error");
                    exit (-1);
                }
            }
        }
    }
    return len;
}

inline int set_nonblocking (int sock_fd)
{
    fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFD, 0) | O_NONBLOCK);
    return 0;
}

inline void add_fd (int epoll_fd, int fd, bool enable_et)
{
    struct epoll_event ep_event;
    ep_event.data.fd = fd;
    ep_event.events = EPOLLIN;
    // 关心 fd 的可读事件，并启用边缘触发模式
    if ( enable_et ) 
        ep_event.events = EPOLLIN | EPOLLET;
    // 将 fd 添加到 epoll_fd 监听列表中，并关联 epoll event.
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ep_event);
    // 将 fd 设为非阻塞模式
    set_nonblocking(fd);
    printf ("fd [%d] add to epoll\n", epoll_fd);
}

#endif