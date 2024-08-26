#include "header.h"
#include "tool.h"

int user_register(int client_fd,char *account,char *password);

query_set *get_user(const char *user_name);


int check_user_login(int client_fd, TransFile *trans, token_info *token)
{
    //去除账号密码中的多余字符
    remove_space(trans->data);
    LOG(LOG_LEVEL_DEBUG,"receive %s",trans->data);
    char **user_log_info = splite_argument(trans->data);
    char user_name[100];
    char user_pwd[100];
    bzero(user_name,100);
    bzero(user_pwd,100);
    strcpy(user_name,user_log_info[0]);
    strcpy(user_pwd,user_log_info[1]);
    free(user_log_info[0]);
    free(user_log_info[1]);
    free(user_log_info);
    LOG(LOG_LEVEL_DEBUG,"user_name = %s,password = %s",user_name,user_pwd);
    //注册
    if(trans->command_no==0)
    {
        LOG(LOG_LEVEL_DEBUG,"receive new register");
        int ret =  user_register(client_fd,user_name,user_pwd);
        if(ret <= 0)
        {
            memset(token,0,sizeof(token_info));
            if(ret == -1)
            {
                send_message(client_fd,"服务器出错，请稍候重试。",trans,false,token);
            }
            else if(ret == 0)
            {
                send_message(client_fd,"账户已存在。",trans,false,token);
            }
            return 1;
        }
        memset(token,0,sizeof(token_info));
        token->user_id = ret;
        sprintf(token->workpath,"/%s",user_name);
        LOG(LOG_LEVEL_DEBUG,"user %s register success",user_name);
        send_message(client_fd,"注册成功！",trans,true,token);
    }
    //登录
    else if(trans->command_no==1)
    {
        LOG(LOG_LEVEL_DEBUG,"receive new login");
        query_set *user_info = get_user(user_name);
        if (user_info == NULL) {
            memset(token,0,sizeof(token_info));
            send_message(client_fd,"服务器繁忙，请稍候重试",trans,false,token);
            return 1;
        }
        user_t *user = (user_t*)(user_info->data);
        //密码无法匹配
        if(!match_password(user_pwd,user->password))
        {
            free_result_set(user_info);
            memset(token,0,sizeof(token_info));
            send_message(client_fd,"帐号或密码不正确，请重试",trans,false,token);
            return 1;
        }
        memset(token,0,sizeof(token_info));
        token->user_id = user->id;
        sprintf(token->workpath,"/%s",user_name);
        LOG_WITH_TIME(LOG_LEVEL_INFO,"user %s login",user_name);
        free_result_set(user_info);
        send_message(client_fd,"密码正确，欢迎登录",trans,true,token);
    }
    return 0;
}
