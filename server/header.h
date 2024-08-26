#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <mysql/mysql.h>
#include <errno.h>
#include <stdarg.h>
#include <shadow.h>
#include <strings.h>
#include <openssl/sha.h>
#include <ctype.h>
#include <l8w8jwt/encode.h>
#include <l8w8jwt/decode.h>
#include "ow-crypt.h"
#include "entity.h"

// tcpConnect
int init_server(const char *ip, const char *port);

// 管理epoll节点
int add_node(const int epfd, const int fd, struct epoll_event *evd);

int remove_node(const int epfd, const int fd);
// tcpConnect end

// 通信格式定义
typedef struct transFile
{
    char data[1024]; // 具体传输数据
    int data_length; // 数据传输具体长度
    int command_no;  // 命令数字，一个值对应一个命令
    bool flag;       // 命令执行是否成功
    char token[1024];// token字段
} TransFile;
// 通信格式结束

// taskQueue 开始
// 节点定义
typedef struct node
{
    TransFile train;
    int netfd;
    struct node *next;
} Node;

// 消息队列定义
typedef struct taskQueue
{
    Node *head;
    Node *tail;
    int queue_size;
    int max_size;
    int short_command_size;
} TaskQueue;

int enqueue(TaskQueue *queue, int netfd,TransFile *train);

int dequeue(TaskQueue *queue, TransFile *train);
// taskQueue 结束

// threadPool相关的结构体以及函数
typedef struct threadPool
{
    pthread_t *thread_id;  // 线程id数组
    int num;               // 线程数量
    TaskQueue q;           // 消息队列
    bool exit;             // 退出标记
    int pipe_in;           // 子线程往主线程的写管道
    pthread_mutex_t mutex; // 共享锁
    pthread_cond_t cond;   // 条件变量
} PthreadPool;

/// @brief 初始化线程池
/// @param num 线程数量 , taskQueue的最大长度为3*num
PthreadPool *init_thread_pool(int num, int pipe_in);

/// @brief 销毁线程池
void destroy_thread_pool(PthreadPool *pool);
// threadPool 结束

extern pthread_t short_thread;


typedef struct
{
    int size;
    void *data;
} query_set;

typedef struct
{
    unsigned long user_id;
    char workpath[1024];
} token_info;

extern char *command[];

// 线程定义
void *thread(void *p);
// cd dest
int change_dir(int client_fd, const char *argument, token_info *token);
// ls
int show_dir(int client_fd, const char *argument, token_info *token);
// ll
int show_dir_detail(int client_fd, const char *argument, token_info *token);
// pwd
int show_path(int client_fd, const char *argument, token_info *token);
// mkdir
int add_dir(int client_fd, const char *argument, token_info *token);
// puts -local -net 将文件存放到file_path
int recv_file(int client_fd, const char *argument, token_info *token);
// gets -net -local将file_path的文件发走
int send_file(int client_fd, const char *argument, token_info *token);
// rm -net 删除file_name的文件
int remove_file(int client_fd, const char *argument, token_info *token);
// 线程定义结束

// 函数指针数组，可以在线程中直接通过command_no取下标传参进行调用。省去了if_else判断
extern int (*deal_command[])(int, const char *,token_info *);

// 可以将相对路径拼接上栈顶元素简化成服务器的绝对路径。
// 要求将相对路径前面加上'/'，如果有'/'则表示是客户端的绝对路径则需要拼上stack[0]为服务器绝对路径
// 需要将返回的字符串进行free。
char *simplifyPath(char *path);

/*
 * log 日志
 */

/// @brief 获取日志文件描述符
/// @return int 返回日志文件描述符
#define LOG_PATH "./log"

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3

#define MAX_LOG_LENGH 2048

void print_log(int level, const char *format, ...);

bool output_control(int level);
int level_to_str(int level, char *level_str);
#define LOG(level, format, ...)                                                                                           \
    do                                                                                                                    \
    {                                                                                                                     \
        if (!output_control(level))                                                                                       \
            break;                                                                                                        \
        char target_level[20] = {0};                                                                                      \
        if (level_to_str(level, target_level) == -1)                                                                      \
            break;                                                                                                        \
        char temp_info[512] = {0};                                                                                        \
        snprintf(temp_info, MAX_LOG_LENGH, "[%s] [%s:%d:%s] %s", target_level, __FILE__, __LINE__, __FUNCTION__, format); \
        print_log(level, temp_info, ##__VA_ARGS__);                                                                       \
                                                                                                                          \
    } while (0)

#define LOG_WITH_TIME(level, format, ...)                                          \
    do                                                                             \
    {                                                                              \
        if (!output_control(level))                                                \
            break;                                                                 \
        time_t rawtime;                                                            \
        struct tm *timeinfo;                                                       \
        time(&rawtime);                                                            \
        timeinfo = localtime(&rawtime);                                            \
        char new_format[1024] = {0};                                               \
        sprintf(new_format, "[%04d-%02d-%02d %02d:%02d:%02d] %s",                  \
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, \
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, (format));  \
        LOG(level, new_format, ##__VA_ARGS__);                                     \
    } while (0)

// 检查命令行参数数量是否符合预期
#define ARGS_CHECK(argc, expected)                         \
    do                                                     \
    {                                                      \
        if ((argc) != (expected))                          \
        {                                                  \
            LOG_WITH_TIME(LOG_LEVEL_ERROR, "args error!"); \
            exit(1);                                       \
        }                                                  \
    } while (0)

// 检查返回值是否是错误标记,若是则打印msg和错误信息
#define ERROR_CHECK(ret, error_flag, msg)        \
    do                                           \
    {                                            \
        if ((ret) == (error_flag))               \
        {                                        \
            LOG_WITH_TIME(LOG_LEVEL_ERROR, msg); \
            exit(1);                             \
        }                                        \
    } while (0)

// 线程错误检查,若ret != 0则打印msg和错误信息
#define THREAD_ERROR_CHECK(ret, msg)             \
    do                                           \
    {                                            \
        if (ret != 0)                            \
        {                                        \
            LOG_WITH_TIME(LOG_LEVEL_ERROR, msg); \
        }                                        \
    } while (0)

#define POINT_CHECK                                               \
    {                                                             \
        printf("%s:%s:%d check\n", __FILE__, __func__, __LINE__); \
    }

/*HASH 加密相关函数声明*/

void encrypt_password(const char *password, char *encrypted);
bool match_password(const char *password, const char *encrypted);
#define CRYPT_OUTPUT_SIZE (7 + 22 + 31 + 1)
#define CRYPT_GENSALT_OUTPUT_SIZE (7 + 22 + 1)
#define CRYPT_MAX_PASSPHRASE_SIZE 72

/*HASH 文件摘要相关声明*/

// 文件摘要字符串长度
#define DIGEST_LEN (64 + 1)
void generate_digest(int fd, char *output);
bool match_digest(int fd, char *digest);

/// @brief 通过读取本目录下的config.conf文件与request条目进行匹配获取对应的配置值并返回
/// @param 传入配置名称
/// @return 以字符串格式返回配置名称所对应的配置值，如果没有找到则返回NULL,使用malloc分配内存，需要手动释放
char *read_config(const char *request);

/*数据库相关声明*/
#define MAX_POOL_SIZE 4
void init_database_pool();
MYSQL *get_connection();
void release_connection(MYSQL *connection);
void destory_database_pool();



query_set *get_result_set(MYSQL *conn, void (*convert)(MYSQL_ROW *row, void *target), unsigned int data_size);
void convert_to_user(MYSQL_ROW *row, void *user);
void convert_to_file(MYSQL_ROW *row,void *file);
void convert_to_userfile(MYSQL_ROW *row,void *user_file);
void free_result_set(query_set *set);
int query(MYSQL *conn, const char *format, ...);

/*jwt token 相关声明*/
#define TOKEN_LEN (1024)

int verify_token(const char *JWT, token_info *info);
int generate_token(token_info *info, char *JWT);

#endif
