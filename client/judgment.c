#include "head.h"

void * threadFunc(void *p){
    printf("I am child thread, \n" );
    TransFile * train =(TransFile *)p;
    printf("command_pth:%s\n",train->data);
    return NULL;
}
/*cd ls ll pwd mkdir puts gets rm*/

void execute_command(char *command,int socket_fd) {
    TransFile *train = (TransFile*)calloc(1,sizeof(TransFile));
    //将命令从回车截断
    int size = 0;
    int command_len = strlen(command);
    for(int i=0;i<command_len;++i){
        if(command[i]==' '){
            size++;
        }
    }
    //0非法，1短命令，2长命令
    int res = 0;
    for(int i=0;i<8;++i){
        //比较命令
        if(strncmp(command, order[i], strlen(order[i]))){
            continue;
        }
        //cd mkdir rm
        if(i==0||i==4||i==7){
            printf("cmd = %s,i=%d,size = %d\n",command,i,size);
            if(size!=1){
                break;
            }
            
            res = 1;
        }
        //ls ll
        else if(i==1||i==2){
            printf("cmd = %s,i=%d,size = %d\n",command,i,size);
            if(size>1){
                break;
            }
            res = 1;
        }
        //puts gets
        else if(i==5||i==6){
            printf("cmd = %s,i=%d,size = %d\n",command,i,size);
            if(size!=2){
                break;
            }
            res = 2;
        }
        //pwd
        else{
            printf("cmd = %s,i=%d,size = %d\n",command,i,size);
            if(size!=0){
                break;
            }
            res = 1;
        }
        train->command_no = i;
        break;
    }
    printf("res = %d\n",res);
    switch (res){
        case 1:
            strcpy(train->data,command);
            train->data_length = command_len;
            strcpy(train->token,global_token);
            send(socket_fd,train,sizeof(TransFile),MSG_NOSIGNAL);
            free(train);
            command_printf(socket_fd);
            break;
        case 2:
            pthread_t pid;
            strcpy(train->data,command);
            train->data_length = command_len;
            strcpy(train->token,global_token);
            int ret = pthread_create(&pid,NULL,transfile,train);
            THREAD_ERROR_CHECK(ret,"thread create error");
            break;
        default:
            printf("非法的命令，请重新输入\n");
            free(train);
    }
}

void command_printf(int socket_fd){
    TransFile train;
    while(1){
        memset(&train,0,sizeof(train));
        int num =recv(socket_fd,&train,sizeof(TransFile),MSG_WAITALL);
        if(num == 0)
        {
            printf("服务器断开连接\n");
        }
        if(train.flag == 0){
            memset(global_token,0,1024);
            strcpy(global_token,train.token);
            printf("%s\n",train.data); 
            break;
        }
        memset(global_token,0,1024);
        strcpy(global_token,train.token);
        printf("%s\n",train.data);
    }
}
