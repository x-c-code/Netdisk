#include "header.h"
#include "tool.h"
#define MAX_EVENT_NUM 1024

pthread_t short_thread = 0;
int pipe_fd[2];
int thread_pipe_fd[2];
void handler_int(int signo);
void handler_alarm(int signo);
void start_work_process(const char *addr, const char *port);
void handler_new_connection(PthreadPool *thread_pool, int listen_fd,int epoll_fd);
void work_process_quit(PthreadPool *thread_pool);
void init_work_condition(const char *addr, const char *port, int *listen_fd, int *epol_fd, PthreadPool **thread_pool);
int main(int argc, char *argv[])
{
    // ARGS_CHECK(argc, 3);
    pid_t pid;
    ERROR_CHECK(pipe(pipe_fd), -1, "pipe failed");
    printf("pipe_fd[0] = %d, pipe_fd[1] = %d\n", pipe_fd[0], pipe_fd[1]);
    ERROR_CHECK((pid = fork()), -1, "fork failed");

    if (pid != 0)
    {
        close(pipe_fd[0]);
        signal(SIGINT, handler_int);
        wait(NULL);
        LOG_WITH_TIME(LOG_LEVEL_INFO, "main process exit");
        exit(0);
    }
    // start_work_process(argv[1], argv[2]);
    start_work_process(read_config("server_ip"), read_config("server_port"));
    return 0;
}

/// @brief INT信号处理函数
void handler_int(int signo)
{
    int quit_flag = 1;
    int ret = write(pipe_fd[1], &quit_flag, sizeof(int));
    ERROR_CHECK(ret, -1, "write int sig to work process failed");
    LOG_WITH_TIME(LOG_LEVEL_INFO, "main process recive a int signal server start close");
    return;
}

/// @brief 子进程开始启动服务器，并配置环境
/// @param addr 监听地址
/// @param port 监听端口
void start_work_process(const char *addr, const char *port)
{
    int epol_fd, listen_fd;
    PthreadPool *thread_pool = NULL;
    struct epoll_event events[MAX_EVENT_NUM];
    struct epoll_event event_listen, event_pipe;

    init_work_condition(addr, port, &listen_fd, &epol_fd, &thread_pool);
    printf("poll size: %d \n", thread_pool->q.queue_size);
    // 添加监听描述符和管道描述符
    add_node(epol_fd, listen_fd, &event_listen);
    add_node(epol_fd, pipe_fd[0], &event_pipe);
    add_node(epol_fd, thread_pipe_fd[0], &event_pipe);

    while (true)
    {
        int ret = epoll_wait(epol_fd, events, MAX_EVENT_NUM, -1);
        ERROR_CHECK(ret, -1, "epoll_wait failed");

        for (int i = 0; i < ret; i++)
        {
            int curr_fd = events[i].data.fd;
            // printf("curr_fd = %d\n", curr_fd);
            //  新连接建立
            if (curr_fd == listen_fd)
            {
                LOG(LOG_LEVEL_DEBUG,"receive new connection request");
                int new_fd = accept(listen_fd, NULL, NULL);
                ERROR_CHECK(new_fd, -1, "accept failed");
                // pthread_mutex_lock(&thread_pool->mutex);
                // enqueue(&thread_pool->q, new_fd);
                // pthread_mutex_unlock(&thread_pool->mutex);
                // pthread_cond_signal(&thread_pool->cond);
                add_node(epol_fd,new_fd,&event_listen);
            }
            // 处理父进程的退出信号
            else if (curr_fd == pipe_fd[0])
            {
                work_process_quit(thread_pool);
            }
            // 处理子线程发送的登录成功或者后续还可能有连接的文件描述符
            else if (curr_fd == thread_pipe_fd[0])
            {
                int client_fd = 0;
                read(thread_pipe_fd[0], &client_fd, sizeof(int));
                add_node(epol_fd, client_fd, &event_listen);
            }
            // 处理已添加的客户端的通信请求
            else
            {
                TransFile train;
                memset(&train,0,sizeof(TransFile));
                int client_fd = curr_fd;
                remove_node(epol_fd, client_fd);
                int recv_num = recv(client_fd,&train,sizeof(TransFile),MSG_WAITALL);
                if(recv_num<=0)
                {
                    close(client_fd);
                    continue;
                }
                LOG(LOG_LEVEL_DEBUG, "new task add queue");
                pthread_mutex_lock(&thread_pool->mutex);
                enqueue(&thread_pool->q, client_fd,&train);
                LOG(LOG_LEVEL_DEBUG, "new task add queue after");
                pthread_mutex_unlock(&thread_pool->mutex);
                pthread_cond_broadcast(&thread_pool->cond);
            }
        }
    }
}

/// @brief ALARM信号处理函数 超时退出
void handler_alarm(int signo)
{
    LOG_WITH_TIME(LOG_LEVEL_WARNING, "work process wait thread quit over time , force exit");
    exit(-1);
    return;
}

/// @brief 处理新连接请求
/// @param thread_pool 线程池指针
/// @param listen_fd 服务器监听描述符
void handler_new_connection(PthreadPool *thread_pool, int listen_fd, int epoll_fd)
{
    
}

/// @brief 工作进程退出
/// @param thread_pool 线程池指针
void work_process_quit(PthreadPool *thread_pool)
{
    LOG_WITH_TIME(LOG_LEVEL_INFO, "work process recive a int signal server start close");
    int quit_flag = 0;
    read(pipe_fd[0], &quit_flag, sizeof(int));

    // 线程池标记退出， 并退出循环，不再监听新连接
    thread_pool->exit = true;
    // 通知所有线程退出
    pthread_cond_broadcast(&thread_pool->cond);
    alarm(5);
    destroy_thread_pool(thread_pool);
    close(thread_pipe_fd[0]);
    close(thread_pipe_fd[1]);

    // 释放数据库连接池
    destory_database_pool();
    exit(0);
}

/// @brief 初始化工作进程的环境
/// @param addr 监听地址
/// @param port 监听端口
/// @param listen_fd 指针，返回监听描述符
/// @param epol_fd 指针，返回epoll描述符
void init_work_condition(const char *addr, const char *port, int *listen_fd, int *epol_fd, PthreadPool **thread_pool)
{
    // 屏蔽INT信号和PIPE信号, ALARM信号
    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, handler_alarm);

    // 初始化服务器
    *listen_fd = init_server(addr, port);
    ERROR_CHECK((*listen_fd), -1, "init_server failed");

    // 创建epoll
    *epol_fd = epoll_create(1);
    ERROR_CHECK((*epol_fd), -1, "epoll_create failed");

    // 创建线程间通信管道
    ERROR_CHECK(pipe(thread_pipe_fd), -1, "pipe failed");

    // 初始化线程池
    *thread_pool = init_thread_pool(4, thread_pipe_fd[1]);

    // 初始化数据库连接池
    init_database_pool();
}
