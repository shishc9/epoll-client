#include "common.hpp"

std::list<int> clients_list;

int main(int argc, char** argv) 
{
    struct sockaddr_in server_addr;
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(SERVER_PORT); //主机序 to 网络序
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("listen_fd lt 0, error occurred.");
        exit(-1);
    }

    printf("listen socket created [%d]\n", listen_fd);

    // sizeof(sockaddr_in) == sizeof(sockaddr)
    // sockaddr use endpoint. sockaddr_in use ip + port.
    if ( bind(listen_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind error.");
        exit(-1);
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen error.");
        exit(-1);
    }

    printf ("start to listen %s_%d\n", SERVER_IP, SERVER_PORT);

    // 创建一个 epoll 实例
    int ep_fd = epoll_create(EPOLL_SIZE);
    if (ep_fd < 0) {
        perror("epoll_fd lt 0, error occurred.");
        exit(-1);
    }

    printf ("epoll created, fd [%d]\n", ep_fd);

    // 将 listen_fd 关联到 epoll 实例上，并启动 edge trigger 边缘模式
    static struct epoll_event events[EPOLL_SIZE];
    add_fd (ep_fd, listen_fd, true);

    while (true) {
        // 循环等待事件发生
        int epoll_event_count = epoll_wait(ep_fd, events, EPOLL_SIZE, -1);
        if (epoll_event_count < 0) {
            perror ("epoll failure.");
            break;
        }

        printf ("epoll_event_count [%d]\n", epoll_event_count);

        for (uint16_t i = 0; i < epoll_event_count; i++) 
        {
            int sock_fd = events[i].data.fd;
            // 新的连接请求
            if (sock_fd == listen_fd) 
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof (struct sockaddr_in);
                int client_fd = accept (listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);

                printf ("client connection from [%s_%d], client_fd [%d]\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), client_fd);

                // 新的请求统计到 epoll 实例中
                add_fd (ep_fd, client_fd, true);

                // 客户端增加
                clients_list.push_back(client_fd);
                printf ("add new client_fd [%d] to epoll\n", client_fd);
                printf ("now there are [%d] clients in the char room\n", (int) clients_list.size());

                printf ("welcome message\n");
                char message[BUF_SIZE];
                bzero (message, BUF_SIZE);
                sprintf (message, SERVER_WELCOME, client_fd);

                int ret = send (client_fd, message, BUF_SIZE, 0);
                if (ret < 0)
                {
                    perror ("send error");
                    exit (-1);
                }
            }
            else 
            {
                // 已连接客户端发送的消息要广播
                int ret = send_broadcast_message (sock_fd);
                if (ret < 0)
                {
                    perror ("error");
                    exit (-1);
                }
            }
        }
    }

    close (listen_fd);
    close (ep_fd);

    return 0;
}