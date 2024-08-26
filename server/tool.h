#include "header.h"
/// @brief 通过读取本目录下的config.conf文件与request条目进行匹配获取对应的配置值并返回
/// @param 传入配置名称
/// @return 以字符串格式返回配置名称所对应的配置值，如果没有找到则返回NULL 
char *read_config(const char *request);

//向客户端发送一条临时信息
void send_message(int client_fd,char *msg,TransFile *train,bool flag,token_info *client_info);

//从系统返回的密码中获取盐值
char *get_sult(const char *shadow_pwd);
//简化一条路径到最简绝对路径
char *simplifyPath(char *path);
//去除一个字符串中多余的空格
void remove_space(char *argument);
//反转字符串
void reverseString(char *str);

//分离同一个字符串中以空格分割的两个参数
char **splite_argument(const char*argument);
char **splite_file_name(const char *path,int *size);
//获取一个路径中最后的文件名。
char *get_file_name(const char *local_path);

void get_user_path(const char *argument,token_info *token,char *user_path);

