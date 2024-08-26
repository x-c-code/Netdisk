#include "header.h"
#include "tool.h"

int query_cd(const char *path,unsigned long user_id);
query_set *query_ls(const char *path, unsigned long user_id);
query_set *query_ll(const char *path, unsigned long user_id);

// cd进入对应路径
// 参数一：客户端文件描述符
// 参数二：需要分析切换进入目录的的路径
int change_dir(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;
    //判断路径合法性
    char user_path[1024]={0};
    get_user_path(argument,token,user_path);
    LOG(LOG_LEVEL_DEBUG,"user_id %lu request change work path to: %s",token->user_id,user_path);
    int ret = query_cd(user_path,token->user_id);
    if(ret<0)
    {
        send_message(client_fd,"server busy,please retry",&trans,false,token);
    }
    else if(ret==0)
    {
        send_message(client_fd,"illegal path, please retry",&trans,false,token);
    }
    else
    {
        //更新工作目录
        memset(token->workpath,0,sizeof(token->workpath));
        strcpy(token->workpath,user_path);
        send_message(client_fd,"change success",&trans,false,token);
    }
    return 0;
}


// 打印当前目录内的文件名
// 参数：当前工作目录
int show_dir(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;
    //判断路径合法性
    char user_path[1024]={0};
    get_user_path(argument,token,user_path);
    LOG(LOG_LEVEL_DEBUG,"user_id %lu request show_dir in path: %s",token->user_id,user_path);
    query_set *res = query_ls(user_path,token->user_id);
    //free(dir_path);
    user_file_t *uf = (user_file_t*)(res->data);
    if(res==NULL)
    {
        send_message(client_fd,"服务器繁忙，请稍候重试",&trans,false,token);
    }
    else
    {
        for(int i=0;i<res->size;++i)
        {
            send_message(client_fd,uf[i].name,&trans,true,token);
        }
        send_message(client_fd,"查找完成",&trans,false,token);
    }
    free_result_set(res);
    return 0;
}

// pwd指令，打印客户的当前工作目录
//解析token之后直接拼上返回就行
int show_path(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;
    LOG(LOG_LEVEL_DEBUG,"user_id %lu request show_path in path: %s",token->user_id,token->workpath);
    send_message(client_fd,token->workpath,&trans,false,token);
    return 0;
}


int show_dir_detail(int client_fd, const char *argument, token_info *token)
{
    TransFile trans;
    char cur_path[512]={0};
    getcwd(cur_path,sizeof(cur_path));
    strcat(cur_path,"/data");
    //判断路径合法性
    char user_path[1024]={0};
    get_user_path(argument,token,user_path);
    LOG(LOG_LEVEL_INFO,"user_id %lu request show_dir_detail in path :%s",token->user_id,user_path);
    query_set *file_detail = query_ll(user_path,token->user_id);
    if(file_detail==NULL)
    {
        LOG(LOG_LEVEL_DEBUG,"file_detail = NULL");
        send_message(client_fd,"系统繁忙，请稍候重试",&trans,false,token);
        return 1;
    }
    query_set *file_name = query_ls(user_path, token->user_id);
    if(file_name == NULL)
    {
        LOG(LOG_LEVEL_DEBUG,"file_name = NULL");
        send_message(client_fd,"系统繁忙，请稍候重试",&trans,false,token);
        free(file_detail);
    }
    printf("%s\n",user_path);
    file_t *f=(file_t*)(file_detail->data);
    user_file_t *uf = (user_file_t*)(file_name->data);
    printf("file_detail->size = %d\n",file_detail->size);
    for(int i=0;i<file_detail->size;++i)
    {
        struct stat stat_buf;
        memset(&stat_buf,0,sizeof(stat_buf));
        // 获取目录项的详细信息
        char path[2048] = {0};
        snprintf(path,2048,"%s/%s",cur_path,f[i].real_path);
        printf("%s\n",path);
        int ret = stat(path, &stat_buf);
        if (ret == -1)
        {
            printf("show_dir_detail : stat error\n");
            continue;
        }
        char type[5] = {0};
        printf("i=%d\n",i);
        printf("f[i].id = %lu\n",f[i].id);
        printf("f[i].type = %d\n",f[i].type);
        if(f[i].type==0)
        {
            strcpy(type,"dir");
        }
        else
        {
            strcpy(type,"file");
        }
        //  文件大小 文件名 文件类型
        char buf[1024]={0};
        snprintf(buf,sizeof(buf),"%s %4ld %s", uf[i].name, stat_buf.st_size, type);
        send_message(client_fd,buf,&trans,true,token);
    }
    send_message(client_fd,"查询完成",&trans,false,token);
    // 关闭目录W
    free_result_set(file_detail);
    free_result_set(file_name);
    LOG(LOG_LEVEL_DEBUG,"show_dir_detail complete");
    return 0;
}
