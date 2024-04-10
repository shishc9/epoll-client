#include "common.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

std::list<int> clients_list;

int main(int argc, char **argv) 
{
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_family = PF_INET;

    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) 
    {
        perror("sock_fd error");
        exit(-1);
    }

    // 连到服务端
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(sockaddr_in)) < 0)
    {
        perror("connect error");
        exit(-1);
    }

    // 管道，父子通信
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0)
    {
        perror("pipe error");
        exit(-1);
    }

    int ep_fd = epoll_create(EPOLL_SIZE);
    if (ep_fd < 0)
    {
        perror ("epoll error");
        exit (-1);
    }

    // 将服务端和管道读端添加到 epoll 实例中
    static struct epoll_event events[2];
    add_fd (ep_fd, sock_fd, true);
    add_fd (ep_fd, pipe_fd[0], true);

    bool client_work = true;
    char message[BUF_SIZE];

    int pid = fork();
    if (pid < 0)
    {
        perror ("fork error");
        exit (-1);
    }
    else if (pid == 0) 
    {
        // 子进程关掉读端
        close (pipe_fd[0]);
        printf ("please input 'exit' to exit the char room\n");

        while (client_work)
        {
            bzero (message, BUF_SIZE);
            fgets (message, BUF_SIZE, stdin);

            // exit
            if (strncasecmp(message, EXIT, strlen(EXIT)) == 0)
            {
                client_work = false;
            }
            else
            {
                // 将输入写到写端
                if (write(pipe_fd[1], message, strlen(message) - 1) < 0)
                {
                    perror ("fork error");
                    exit (-1);
                }
            }
        }
    }
    else
    {
        // 父进程关掉写端
        close (pipe_fd[1]);

        while (client_work)
        {
            int epoll_events_count = epoll_wait (ep_fd, events, 2, -1);
            for (int i = 0; i < epoll_events_count; i++)
            {
                bzero (message, BUF_SIZE);
                // 服务端广播消息
                if (events[i].data.fd == sock_fd)
                {
                    int ret = recv (sock_fd, message, BUF_SIZE, 0);
                    if (ret == 0) 
                    {
                        printf ("server closed connection: %d\n", sock_fd);
                        close (sock_fd);
                        client_work = false;
                    }
                    else 
                        printf ("%s\n", message);
                }
                else 
                {
                    // 管道读端有数据
                    int ret = read (events[i].data.fd, message, BUF_SIZE);
                    if (ret == 0)
                    {
                        client_work = 0;
                    }
                    else
                        // 发给服务器
                        send (sock_fd, message, BUF_SIZE, 0);
                }
            }
        }
    }

    if (pid)
    {
        close (pipe_fd[0]);
        close (sock_fd);
    }
    else
    {
        close (pipe_fd[1]);
    }

    return 0;
}