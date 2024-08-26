#include "header.h"

int init_server(const char *ip, const char *port)
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd <= 0)
    {
        return sock_fd;
    }
    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(atoi(port));
    int ret = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        close(sock_fd);
        return ret;
    }

    if (listen(sock_fd, 10) < 0)
    {
        close(sock_fd);
        return -1;
    }
    LOG_WITH_TIME(LOG_LEVEL_INFO, "server start success");
    return sock_fd;
}
/// @brief 添加epoll监听描述符
/// @param epfd epoll描述符 fd 监听描述符, evd 事件结构体指针
/// @return int 添加成功返回0, 失败返回-1
int add_node(const int epfd, const int fd, struct epoll_event *evd)
{
    if (evd == NULL)
    {
        evd = calloc(1, sizeof(struct epoll_event));
    }

    evd->data.fd = fd;
    evd->events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, evd);
    ERROR_CHECK(ret, -1, "epoll_ctl failed");
    //printf("add node %d in epfd: %d\n", fd, epfd);
    return ret;
}

int remove_node(const int epfd, const int fd)
{
    return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}
