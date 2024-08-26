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
#define ARGS_CHECK(argc, num){if(argc != num){fprintf(stderr, "args error !\n"); return -1;} }
#define ERROR_CHECK(ret, num, msg){if(ret==num){perror(msg); return -1;}}
#define THREAD_ERROR_CHECK(ret, msg){if(ret!=0){fprintf(stderr, "%s:%s \n", msg,strerror(ret));}}
#define BUFFER_SIZE (4096 * 1024)
typedef  struct trainsFile{
    char data[1024];
    int data_length;
    int command_no;
    bool flag;
    char token[1024];
}TransFile;

extern char global_token[1024];


extern char *order[];
char *trim(char *str);
int initTcpSocket(int *socketfd,char *ip,char *port);

void execute_command(char *command,int socket_fd);
int puts_File(int fd, int socket_fd); 
int gets_File(int fd, int socket_fd);
void command_printf(int socket_fd);

char *simplifyPath(char *path);
void reverseString(char *str);
char *get_file_name(const char *local_path);
int getparameter(char *key, char *value);
bool login(int socket_fd);
bool register_client(int socket_fd);
void generate_digest(int fd, char *output);
void string_to_hex(const unsigned char *input, int length, char *output);
//线程启动函数头文件
void * transfile(void *p);

//去除多余空格
void remove_space(char *argument);

//分离同一个字符串中以空格分割的两个参数
char **splite_argument(const char*argument);
char **splite_file_name(const char *path,int *size,char target);
#endif
