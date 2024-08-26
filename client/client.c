#include "head.h"

char global_token[1024]={0};
char *order[] ={"cd","ls","ll","pwd","mkdir","puts","gets","rm"};

int main(int argc,char *argv[]){
    char command[1024] = {0};
    char client_input[10] = {0};
    int socket_fd;
    char ip[100] = {0};
    char port[100] = {0};

    //获取ip和port的配置文件信息
    getparameter("client_ip", ip);
    getparameter("port", port);
    remove_space(ip);
    remove_space(port);
    printf("ip = %s,port = %s\n",ip,port);
    //与客户端进行Tcp连接
    initTcpSocket(&socket_fd,ip,port);
    printf("欢迎使用网盘，请输入你的操作!\n");
    //监听用户输入;
    while(1){
        printf("输入1:用户登录\n");
        printf("输入2:用户注册\n");
        memset(client_input,0,sizeof(client_input));
        read(STDIN_FILENO,client_input,sizeof(client_input));        
        if(strncmp(client_input,"1",1) == 0){
            if(login(socket_fd) == true){
                break;
            }
            else{
                printf("请重新输入\n");
                continue;
            }        
        }
        else if(strncmp(client_input,"2",1) == 0){
            if(register_client(socket_fd) == true){
                continue;
            }
            else{
                printf("请重新输入\n");
                continue;
            }
        }
        else{
            printf("请重新输入\n");
        }
    }
    printf("global_token = %s\n",global_token);
    while(1){
        //监听命令输入
        memset(command,0,sizeof(command));
        int res_file_num = read(STDIN_FILENO,command,sizeof(command));
        if(res_file_num == 0){
            break;
        }
        remove_space(command);
        //判断命令是否合法，合法送到服务器执行
        execute_command(command,socket_fd);
    }
    return 0;
} 
