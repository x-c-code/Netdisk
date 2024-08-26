#include "header.h"
#include "tool.h"
 int (*deal_command[])(int, const char *, token_info *) =
     {change_dir, show_dir, show_dir_detail, show_path, add_dir, recv_file, send_file, remove_file};

char *command[] = {"cd", "ls", "ll", "pwd", "mkdir", "puts", "gets", "rm",NULL};


int check_user_login(int client_fd, TransFile *trans, token_info *token);
int connect_with_client(int client_fd, TransFile *train,token_info *token);

void *thread(void *p)
{
    PthreadPool *pool = (PthreadPool *)p;
    TransFile trans;
    token_info token;
    pthread_t self = pthread_self();
    int command_no = 0;
    // int count= 0;
    while (1)
    {
        // count++;
        // if(count>10)
        // {
        //     break;
        // }
        // 获取任务
        pthread_mutex_lock(&pool->mutex);
        LOG(LOG_LEVEL_DEBUG,"q.short_command_size = %d",pool->q.short_command_size);
        while (pool->q.queue_size <= 0||(self==short_thread&&pool->q.short_command_size==0))
        {
            
            // 检测是否退出
            if (pool->exit == true)
            {
                pthread_mutex_unlock(&pool->mutex);
                pthread_cond_signal(&pool->cond);
                LOG_WITH_TIME(LOG_LEVEL_INFO, "thread [%lu] exit", pthread_self());
                pthread_exit(NULL);
            }
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        int net_fd = dequeue(&pool->q,&trans);
        pthread_mutex_unlock(&pool->mutex);
        if(net_fd==-1)
        {
            LOG(LOG_LEVEL_DEBUG,"短线程未取到任务");
            //pthread_cond_broadcast(&pool->cond);
            continue;
        }
        //count = 0;
        // memset(&trans, 0, sizeof(TransFile));
        // memset(&token,0,sizeof(token_info));
        // int size = recv(net_fd,&trans,sizeof(trans),MSG_WAITALL);
        // if(size == 0)
        // {
        //     LOG(LOG_LEVEL_DEBUG,"net_fd %d disconnected");
        //     close(net_fd);
        //     continue;
        // }
        LOG(LOG_LEVEL_DEBUG,"new connection net_fd is %d data is %s",net_fd,trans.data);
        LOG(LOG_LEVEL_DEBUG, "data_len: %d\n, command_no: %d\n, flag:%d\n, token:%s", 
        trans.data_length, trans.command_no, trans.flag, trans.token);
        // 获取当前时间，供后续判断是否超时
        time_t curtime;
        time(&curtime);
        command_no = trans.command_no;
        // 首先判断是否携带token
        if (strlen(trans.token)==0)
        {
            check_user_login(net_fd, &trans,&token);
        }
        else
        {
            //  事务处理结束之后断开连接。
            int ret = verify_token(trans.token,&token);
            //token解析错误
            if(ret!=0)
            {
                LOG(LOG_LEVEL_DEBUG,"thread %ld deal netfd %d verify_token invilied",pthread_self(),net_fd);
                send_message(net_fd,"请重新登录",&trans,false,&token);
            }
            else if(-1==connect_with_client(net_fd,&trans,&token))
            {
                memset(&token,0,sizeof(token_info));
                send_message(net_fd,"连接关闭",&trans,false,&token);
                //close(net_fd);
            }
        }
        LOG_WITH_TIME(LOG_LEVEL_INFO, "thread %ld deal client %d request complete", pthread_self(), net_fd);
        //close(net_fd);
        if(command_no!=5&&command_no!=6)
        {
            write(pool->pipe_in,&net_fd,sizeof(int));
        }
    }
    return NULL;
}


int connect_with_client(int client_fd, TransFile *train, token_info *token)
{
    LOG_WITH_TIME(LOG_LEVEL_INFO, "thread %ld receive user %lu request : %s", pthread_self(),token->user_id, train->data);

    // 去除多余重复空格
    remove_space(train->data);
    LOG_WITH_TIME(LOG_LEVEL_INFO, "after deal request : %s", train->data);

    int command_no;
    char cmd[10] = {0};
    char argument[train->data_length];
    memset(argument, 0, sizeof(argument));
    char **temp = splite_argument(train->data);
    if(temp!=NULL)
    {
        strcpy(cmd,temp[0]);
        strcpy(argument,temp[1]);
        free(temp[0]);
        free(temp[1]);    
        free(temp);
    }
    else
    {
        strcpy(cmd,train->data);
    }
    LOG(LOG_LEVEL_DEBUG,"command = %s,argument = %s",cmd,argument);
    for(command_no=0;command[command_no]!=NULL;++command_no)
    {
        if(strcmp(cmd,command[command_no])==0)
        {
            break;
        }
    }
    if(command[command_no]==NULL)
    {
        send_message(client_fd,"无效的命令",train,false,token);
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG,"thread: command is %s ;argument = %s\n", command[command_no], argument);
    deal_command[command_no](client_fd, argument, token);
    return 0;
}
