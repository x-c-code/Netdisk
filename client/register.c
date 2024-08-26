#include "head.h"

bool register_client(int socket_fd){
    TransFile train_client;
    memset(&train_client,0,sizeof(TransFile));
    char account[20] = {0};
    char password[20] = {0};
    printf("注册用户:\n");
    printf("请输入账号:\n"); 
    read(STDIN_FILENO,account,sizeof(account));
    printf("请输入账号密码:\n");
    read(STDIN_FILENO,password,sizeof(password));
    char *act = strtok(account, "\n");
    char *psw = strtok(password, "\n");
    char *info  = strcat(act," ");
    char *client_info  = strcat(info,psw);
    strcpy(train_client.data, client_info);
    train_client.data_length = strlen(train_client.data);
    train_client.command_no = 0;
    train_client.flag = 1;
    send(socket_fd,&train_client,sizeof(train_client),MSG_NOSIGNAL);
    recv(socket_fd,&train_client,sizeof(train_client),MSG_WAITALL);
    if(train_client.flag == true ){
        printf("%s\n",train_client.data);
        return true;
    }else{
        printf("%s\n",train_client.data);
        return false;
    }
}
