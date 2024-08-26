#include "head.h"

bool login(int socket_fd){
    TransFile train_client;
    memset(&train_client,0,sizeof(TransFile));
    char account[20] = {0};
    char password[20] = {0};
    printf("账号登录:\n"); 
    printf("请输入账号:\n");
    read(STDIN_FILENO,account,sizeof(account));
    printf("请输入账号密码:\n");
    read(STDIN_FILENO,password,sizeof(password));
    int account_len = strlen(account)-1;
    int password_len = strlen(password)-1;
    account[account_len] = '\0';
    password[password_len] = '\0';
    sprintf(train_client.data,"%s %s",account,password);
    train_client.data_length = strlen(train_client.data);
    train_client.command_no = 1;
    train_client.flag = 0;
    memset(train_client.token, 0, sizeof(train_client.token));
    printf("data send |%s|\n",train_client.data);
    send(socket_fd,&train_client,sizeof(train_client),MSG_NOSIGNAL);
    memset(&train_client,0,sizeof(TransFile));
    int res_num = recv(socket_fd,&train_client,sizeof(TransFile),MSG_WAITALL);
    if(res_num == 0){
        printf("服务端连接关闭\n");
        return false;
    }
    if(train_client.flag == true){
        printf("%s\n",train_client.data);
        memset(global_token,0,sizeof(global_token));
        strcpy(global_token,train_client.token);
        return true;
    }else{
        printf("%s\n",train_client.data);
        return false;
    }

}
