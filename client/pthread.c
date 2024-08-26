#include "head.h"

void * transfile(void *p){
    TransFile *trans = (TransFile*)p;
    printf("thread start to deal %s\n",trans->data);
    int sock_fd = 0;
    char ip[20] = {0};
    char port[20] = {0};
    getparameter("client_ip",ip);
    getparameter("port",port);
    printf("ip = %s,port = %s\n",ip,port);
    char cmd[256] = {0};
    strcpy(cmd,trans->data);

    char *command = strtok(cmd," ");
    char *argument1 = strtok(NULL," ");
    char *argument2 = strtok(NULL,"\n");
    printf("command = %s\nargument1 = %s\nargument2 = %s\ntrans.command_no = %d\n",command,argument1,argument2,trans->command_no);
    char *file_name = get_file_name(argument1);
    char local_path[1024] = {0};
    int fd = 0;
    //说明这是puts，需要将本地文件进行上传，本地路径跟文件名都在argument1中
    if(trans->command_no==5){
        //这是绝对路径
        if(argument1[0]=='/'){
            strcpy(local_path,argument1);
        }
        //这是相对路径
        else{
            //获取当前工作目录
            getcwd(local_path,sizeof(local_path));
            sprintf(local_path,"%s/%s",local_path,argument1);
            //简化获得真实路径
            char *temp = simplifyPath(local_path);
            memset(local_path,0,sizeof(local_path));
            strcpy(local_path,temp);
            free(temp);
        }
        fd = open(local_path,O_RDONLY);
    }
    //说明这是gets，文件名在argument1中，本地路径为argument2
    else if(trans->command_no==6){
        if(argument2[0]=='/'){
            strcpy(local_path,argument2);
            strcat(local_path,file_name);
        }
        else{
            //获取当前工作目录
            getcwd(local_path,sizeof(local_path));
            //拼接相对路径
            sprintf(local_path,"%s/%s%s",local_path,argument2,file_name);
            printf("thread: local_path = \n|%s|\n",local_path);
            char *temp_local_path = simplifyPath(local_path);
            printf("thread: temp_localpath = \n|%s|\n",temp_local_path);
            memset(local_path,0,sizeof(local_path));
            strcpy(local_path,temp_local_path);
            free(temp_local_path);
        }
        
        fd = open(local_path,O_RDWR|O_CREAT,0644);
    }
    printf("thread: real_path = %s\n",local_path);
    printf("thread: fd = %d\n",fd);
    if(fd>0){
        initTcpSocket(&sock_fd,ip,port);
        strcpy(trans->token,global_token);
        printf("trans.data = %s\n",trans->data);
        send(sock_fd,trans,sizeof(TransFile),MSG_NOSIGNAL);
        if(trans->command_no==5){
            puts_File(fd,sock_fd);
        }
        else if(trans->command_no==6){
            gets_File(fd,sock_fd);
        }
        close(fd);
        close(sock_fd);
    }
    else{
        printf("open error:%s\n",strerror(errno));
    }
    free(file_name);
    free(trans);
    return NULL;
}     


int puts_File(int fd, int socket_fd){
    TransFile train;
    memset(&train,0,sizeof(TransFile));
    off_t file_size = lseek(fd,0,SEEK_END);
    train.data_length = file_size;
    generate_digest(fd,train.data);
    printf("开始发送文件哈希值和文件大小\n");
    send(socket_fd,&train,sizeof(TransFile),MSG_NOSIGNAL);
    memset(&train,0,sizeof(TransFile));
    printf("开始接收服务端文件大小\n");
    recv(socket_fd,&train,sizeof(TransFile),MSG_WAITALL);
    if(train.flag==false){
        printf("thread: %s\n",train.data);
        memset(global_token,0,sizeof(global_token));
        strcpy(global_token,train.token);
        return 0;
    }
    off_t server_size = train.data_length;
    lseek(fd,server_size,SEEK_SET);
    printf("开始接收文件\n");
    sendfile(socket_fd,fd,&server_size,file_size-server_size);
    recv(socket_fd,&train,sizeof(TransFile),MSG_WAITALL);
    return 0;
}


//下载文件
int gets_File(int fd,int socket_fd){
    TransFile trans;
    memset(&trans,0,sizeof(TransFile));
    printf("开始接收文件大小\n");
    recv(socket_fd,&trans,sizeof(TransFile),MSG_WAITALL);
    if(trans.flag==false){
        printf("thread: %s\n",trans.data);
        memset(global_token,0,sizeof(global_token));
        strcpy(global_token,trans.token);
    }
    else{
        off_t file_size = trans.data_length;
        off_t cur_size = lseek(fd,0,SEEK_END);
        long page_size = sysconf(_SC_PAGE_SIZE);
        cur_size = (cur_size/page_size)*page_size;
        trans.data_length = cur_size;
        send(socket_fd,&trans,sizeof(TransFile),MSG_NOSIGNAL);
        printf("thread: 发送本机文件长度: %d\n",trans.data_length);
        ftruncate(fd,file_size);
        printf("thread: 扩展文件大小成功\n");
        ssize_t recv_num = 0;
        char buf[4096] = {0};
        while(cur_size<file_size){
            printf("cur_size = %ld\n",cur_size);
            recv_num = read(socket_fd,buf,sizeof(buf));
            printf("thread: recv_num = %ld\n",recv_num);
            if(recv_num<=0){
                printf("thread: 关闭连接\n");
                return 0;
            }
            off_t count = cur_size+recv_num>file_size?file_size-cur_size:recv_num;
            void *src_area = mmap(NULL,count,PROT_READ|PROT_WRITE,MAP_SHARED,fd,cur_size);
            if(src_area==MAP_FAILED){
                perror("mmap\n");
                return 0;
            }
            memcpy(src_area,buf,count);
            munmap(src_area,count);
            cur_size+=count;
        }
        send(socket_fd,&trans,sizeof(trans),MSG_NOSIGNAL);
    }
    return 0;
}
