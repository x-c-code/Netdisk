#include "header.h"
#include "tool.h"

int query_mkdir(const char *path, char *dir_name,unsigned long user_id);
int query_puts(const char *path, char *file_name,char *hash_value,unsigned long user_id);
int query_gets(const char *path, char *file_name,char *hash_value,unsigned long user_id);
int query_update(const char *path,char *hash_value, unsigned long user_id);
int query_remove(const char *path, char *file_name,unsigned long user_id);

int recv_file(int client_fd,const char *argument, token_info *token)
{
    TransFile trans;
    memset(&trans, 0, sizeof(trans));

    
    // 服务器存储文件的真实路径
    char real_path[1024] = {0};
    getcwd(real_path,sizeof(real_path));
    strcat(real_path,"/data");
    // 客户当前的工作路径,token解析
    char user_path[1024] = {0};
    
    // 需要将argument进行切分，获取网络路径，在上一步已经将多余重复空格进行了删除
    // 所以只需要按空格进行切分，获取后面的那个即可
    char **res = splite_argument(argument);
    char *file_name = get_file_name(res[0]);
    get_user_path(res[1],token,user_path);
    free(res[0]);
    free(res[1]);
    free(res);
    LOG(LOG_LEVEL_DEBUG,"argument = %s",argument);
    //接受文件哈希值和文件长度
    // memset(&trans,0,sizeof(TransFile));
    int recv_size = recv(client_fd,&trans,sizeof(TransFile),MSG_WAITALL);
    char hash_value[1024]={0};
    strcpy(hash_value,trans.data);
#if 0
    LOG(LOG_LEVEL_DEBUG,"开始睡眠");
    sleep(60);
    LOG(LOG_LEVEL_DEBUG,"结束睡眠");
#endif
    //获取客户端要上传的文件名并拼接到服务器路径后。
    int ret = query_puts(user_path,file_name,hash_value,token->user_id);
    LOG(LOG_LEVEL_INFO,"user_id %lu request puts %s hash_value is %s\n in path :%s, ret = %d",token->user_id,file_name,trans.data,user_path,ret);
    free(file_name);
    switch (ret)
    {
    case -2:
        send_message(client_fd,"服务器繁忙，请稍候重试",&trans,false,token);
        return 0;    
    case -1:
        send_message(client_fd,"无效的路径，请重试",&trans,false,token);
        return 0;
    case 0:
        send_message(client_fd,"该目录下有同名文件，请检查",&trans,false,token);
        return 0;
    case 2:
        send_message(client_fd,"上传成功",&trans,false,token);
        return 0;
    }
    strcat(real_path,"/");
    strcat(real_path,trans.data);
    //打开文件，如果打不开直接返回报错。
    int fd = open(real_path,O_RDWR|O_CREAT,0666);

    //接收文件长度

    int file_size = trans.data_length;
    if (fd < 0)
    {
        int errsv = errno;
        printf("%s\n", strerror(errsv));
        send_message(client_fd,strerror(errsv),&trans,false,token);
        return -1;
    }
    // 发送已经有的长度,如果是0则代表里面没有数据。
    off_t offset = lseek(fd, 0, SEEK_END);
    long page_size = sysconf(_SC_PAGE_SIZE);
    offset = (offset/page_size)*page_size;
    trans.flag = true;
    trans.data_length = (int)offset;
    send(client_fd, &trans, sizeof(trans), MSG_NOSIGNAL);
    ftruncate(fd,file_size);
    int recv_num;
    memset(&trans, 0, sizeof(trans));
    char buf[4096] = {0};
    while (offset < file_size){
        recv_num = recv(client_fd, buf, sizeof(buf),0);
        if (recv_num <= 0){
            LOG(LOG_LEVEL_DEBUG,"客户端断开连接");
            break;
        }
        LOG(LOG_LEVEL_DEBUG,"offset = %ld, recv_num = %ld,file_size = %ld",offset,recv_num,file_size);
        off_t count = offset+recv_num>file_size?file_size-offset:recv_num;
        void *src_area = mmap(NULL,count,PROT_READ|PROT_WRITE,MAP_SHARED,fd,offset);
        if(src_area==MAP_FAILED){
            LOG(LOG_LEVEL_DEBUG,"mmap:%s",strerror(errno));
            break;
        }
        memcpy(src_area,buf,count);
        munmap(src_area,count);
        offset += count;
        memset(&trans, 0, sizeof(trans));
    }
    send(client_fd,&trans,sizeof(TransFile),MSG_NOSIGNAL);
    close(client_fd);
    close(fd);
    if(offset==file_size)
    {
        query_update(user_path,hash_value,token->user_id);
    }
    return 0;
}

// server_path client_path
int send_file(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;

    //服务器存放文件路径
    char data_path[1024] = {0};
    getcwd(data_path,sizeof(data_path));
    strcat(data_path,"/data/");

    //获取客户端想要的文件名和想操作的服务器路径
    //temp[0]就是

    char **temp = splite_argument(argument);
    char user_path[1024] = {0};
    get_user_path(temp[0],token,user_path);
    free(temp[0]);
    free(temp[1]);
    free(temp);

    char *file_name = get_file_name(user_path);
    for(int i=strlen(user_path)-1;i>0;--i)
    {
        if(user_path[i]=='/')
        {
            user_path[i] = '\0';
            break;
        }
    }
    
    LOG(LOG_LEVEL_DEBUG,"user %d : gets file %s from path %s to local",token->user_id,file_name,user_path);
#if 0
    LOG(LOG_LEVEL_DEBUG,"开始休眠");
    sleep(60);
    LOG(LOG_LEVEL_DEBUG,"结束休眠");
#endif
    //获取服务端存放文件的哈希值
    char hash_value[1024] = {0};
    int ret = query_gets(user_path,file_name,hash_value,token->user_id);
    free(file_name);
    switch(ret)
    {
        case -1:
            send_message(client_fd,"服务器错误",&trans,false,token);
            close(client_fd);
            return 0;
        case 0:
            send_message(client_fd,"不合法的路径或文件不存在",&trans,false,token);
            close(client_fd);
            return 0;
    }
    strcat(data_path,hash_value);
    LOG(LOG_LEVEL_DEBUG,"data_path = %s",data_path);
    int fd = open(data_path,O_RDWR);
    if(fd<=0)
    {
        send_message(client_fd,"服务器错误",&trans,false,token);
    }
    off_t file_size = lseek(fd,0,SEEK_END);
    memset(&trans,0,sizeof(TransFile));
    trans.data_length = file_size;
    trans.flag = true;
    send(client_fd,&trans,sizeof(TransFile),MSG_NOSIGNAL);
    LOG(LOG_LEVEL_DEBUG,"向客户端client %d 发送文件大小 %ld",client_fd,file_size);
    LOG(LOG_LEVEL_DEBUG,"准备接收客户端文件大小");
    memset(&trans,0,sizeof(TransFile));
    int recv_size = recv(client_fd,&trans,sizeof(TransFile),MSG_WAITALL);
    if(recv_file<=0)
    {
        LOG(LOG_LEVEL_DEBUG,"客户端 %d 关闭连接",client_fd);
        close(client_fd);
        return 0;
    }
    off_t client_size = trans.data_length;
    LOG(LOG_LEVEL_DEBUG,"收到客户端 %d 文件大小 %ld",client_fd,client_size);
    lseek(fd,client_size,SEEK_SET);
    LOG(LOG_LEVEL_DEBUG,"文件准备发送 %ld",file_size-client_size);
    int send_size = sendfile(client_fd,fd,0,file_size-client_size);
    LOG(LOG_LEVEL_DEBUG,"文件发送 %d",send_size);
    memset(&trans,0,sizeof(TransFile));
    recv(client_fd,&trans,sizeof(TransFile),MSG_WAITALL);
    close(client_fd);
    return 0;
}

int remove_file(int client_fd, const char *argument, token_info *token)
{
    // 通过根据argument的路径获取绝对路径并进行删除。
    TransFile trans;
    //服务器存放文件的绝对路径
    char cur_path[1024]={0};
    getcwd(cur_path,sizeof(cur_path));
    strcat(cur_path,"/data");
    
    //判断客户端传入路径合法性
    char user_path[1024]={0};
    get_user_path(argument,token,user_path);
    char *file_name = get_file_name(argument);
    for(int i=strlen(user_path)-1;i>0;--i)
    {
        if(user_path[i]=='/')
        {
            user_path[i]='\0';
            break;
        }
    }
    LOG(LOG_LEVEL_INFO,"user_id %lu request remove_file in path :%s",token->user_id,user_path);

    int ret = query_remove(user_path,file_name,token->user_id);
    if(ret == -1)
    {
        send_message(client_fd,"系统繁忙，请稍候重试",&trans,false,token);
        return -1;
    }
    if(ret == 1)
    {
        send_message(client_fd,"请输入正确路径",&trans,false,token);
        return -1;
    }
    if(ret == 2)
    {
        send_message(client_fd,"文件夹非空，请清空后重试",&trans,false,token);
        return -1;
    }
    send_message(client_fd, "删除成功",&trans,false,token);

    return 0;
}

int add_dir(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;

    //判断客户端传入路径合法性
    char user_path[1024]={0};
    get_user_path(argument,token,user_path);
    char *file_name = get_file_name(argument);
    for(int i=strlen(user_path)-1;i>0;--i)
    {
        if(user_path[i]=='/')
        {
            user_path[i]='\0';
            break;
        }
    }
    LOG(LOG_LEVEL_INFO,"user_id %lu request add dir %s in path :%s",token->user_id,file_name,user_path);

    int ret = query_mkdir(user_path,file_name,token->user_id);
    if(ret == -1)
    {
        send_message(client_fd,"系统繁忙，请稍候重试",&trans,false,token);
        return -1;
    }
    if(ret == 0 )
    {
        send_message(client_fd,"请输入正确路径",&trans,false,token);
        return -1;
    }
    if(ret == 1)
    {
        send_message(client_fd,"当前目录下已有同名文件",&trans,false,token);
        return -1;
    }
    send_message(client_fd,"创建成功",&trans,false,token);
    return 0;
}
