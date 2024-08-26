#include "header.h"

char *read_config(const char *request)
{
    FILE *conf = fopen("config.conf","r");
    char buf[1024] = {0};
    int request_len = strlen(request);
    char *need=NULL;
    while(NULL!=(need = fgets(buf,sizeof(buf),conf)))
    {
        if(strncmp(buf,request,request_len)==0)
        {
            break;
        }
        memset(buf,0,sizeof(buf));
    }
    fclose(conf);
    if(need == NULL)
    {
        return NULL;
    }
    need = strchr(buf,'=')+1;
    int ret_len = strlen(need)-1;
    char *ret = (char *)calloc(ret_len+1,sizeof(char));
    strncpy(ret,need,ret_len);
    return ret;
}

void send_message(int client_fd,char *msg,TransFile *train,bool flag,token_info *client_info)
{
    memset(train, 0, sizeof(TransFile));
    int msg_len = strlen(msg);
    strncpy(train->data,msg,msg_len);
    train->data_length = msg_len;
    train->flag = flag;
    if(client_info->user_id!=0)
    {
        generate_token(client_info,train->token);
    }
    send(client_fd,train,sizeof(TransFile),MSG_NOSIGNAL);
    LOG(LOG_LEVEL_DEBUG,"send_data = %s",train->data);
}

//从系统返回的密码中获取盐值
char *get_sult(const char *shadow_pwd)
{
    int pwd_len = strlen(shadow_pwd);
    const char *end = shadow_pwd+pwd_len-1;
    while(*end!='$')
    {
        end--;
    }
    int sult_len = end-shadow_pwd+1;
    char *sult = (char *)calloc(sult_len+1,sizeof(char));
    strncpy(sult,shadow_pwd,sult_len);
    return sult;
}

char *simplifyPath(char *path)
{
    int pathLen = strlen(path);
    char *name = (char *)calloc(pathLen + 1, sizeof(char));
    int nameIdx = 0;
    int list[pathLen];
    int listIdx = 0;
    memset(list, 0, sizeof(list));
    name[nameIdx++] = '/';
    list[listIdx++] = 0;
    int lastSlash = 0;

    for (int i = 1; i <= pathLen; ++i)
    {
        if (i < pathLen && path[i] != '/')
        {
            name[nameIdx++] = path[i];
        }
        else if (i == pathLen || path[i] == '/')
        {
            // 判断两个斜杠之间的距离
            int nameLen = i - lastSlash;
            lastSlash = i;
            // 如果紧挨着则更新最近斜杠并判断下一位。
            if (nameLen == 1)
            {
                continue;
            }
            // 如果中间间隔一位判断间隔位是不是'.'，如果是则nameidx跳转到上个斜杠
            // 进行下一位的判断，如果不是'.'则正常记录
            else if (nameLen == 2)
            {
                if (path[i - 1] == '.')
                {
                    nameIdx = list[listIdx - 1] + 1;
                }
                else
                {
                    name[nameIdx++] = '/';
                    list[listIdx++] = nameIdx - 1;
                }
            }
            // 如果中间间隔两位判断间隔位是不是两个'.'，如果是则nameidx跳转到上上个斜杠
            // 如果斜杠位置不够跳则跳转到最开头。
            // 进行下一位的判断，如果不是'.'则正常记录。
            else if (nameLen == 3)
            {
                if (path[i - 1] == '.' && path[i - 2] == '.')
                {
                    if (listIdx <= 1)
                    {
                        listIdx = 1;
                        nameIdx = 1;
                    }
                    else
                    {
                        nameIdx = list[--listIdx - 1] + 1;
                    }
                }
                else
                {
                    name[nameIdx++] = '/';
                    list[listIdx++] = nameIdx - 1;
                }
            }
            // 如果中间间隔三位或者以上则正常记录。
            else
            {
                name[nameIdx++] = '/';
                list[listIdx++] = nameIdx - 1;
            }
        }
    }
    int num = 1;
    if (nameIdx == 1)
    {
        num = 0;
    }
    name[nameIdx - num] = '\0';
    return name;
}

void remove_space(char *argument)
{
    int argument_len = strlen(argument);
    char temp[argument_len+1];
    memset(temp,0,sizeof(temp));
    int j=0;

    for (int i=0;i<argument_len;++i)
    {
        if(i==0&&argument[i]==' ')
        {
            continue;
        }
        if(argument[i]==' '&&argument[i-1]==' ')
        {
            continue;
        }
        if(argument[i]=='\n')
        {
            continue;
        }
        temp[j++] = argument[i];
    }
    if(temp[j-1]==' ')
    {
        temp[j-1] = '\0';
    }
    memset(argument,0,argument_len);
    strncpy(argument,temp,j);
}

void reverseString(char *str)
{
    int length = strlen(str);
    char temp;
    for (int i = 0; i < length / 2; i++)
    {
        temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}
//分离同一个字符串中以空格分割的两个参数
char **splite_argument(const char*argument)
{
    char **ret = (char**)malloc(sizeof(char*)*2);
    int argument_len = strlen(argument);
    char *space_loaction = strchr(argument,' ');
    if(space_loaction==NULL)
    {
        return NULL;
    }
    int argument1_len = space_loaction - argument;
    while(*space_loaction==' ')
    {
        space_loaction++;
    }
    int argument2_len = argument_len-(space_loaction - argument);
    char *ret1 = (char *)calloc(argument1_len+1,sizeof(char));
    char *ret2 = (char *)calloc(argument2_len+1,sizeof(char));
    strncpy(ret1,argument,argument1_len);
    strncpy(ret2,space_loaction,argument2_len);
    ret[0] = ret1;
    ret[1] = ret2;
    return ret;
}
char **splite_file_name(const char *path,int *size)
{
    int path_len = strlen(path);
    char *dir_name[path_len];
    memset(dir_name,0,sizeof(char*)*path_len);
    int dirsize = 0;
    const char *start = path;
    const char *end = path;
    //如果start跟end相等说明是在开头，直接让end++
    while((end-path)<path_len)
    {
        if(*end=='/')
        {
            if(end==start)
            {
                end++;
                continue;
            }
            int dir_name_len = end-start-1;
            dir_name[dirsize] = (char*)calloc(dir_name_len+1,sizeof(char));
            strncpy(dir_name[dirsize],start+1,dir_name_len);
            dirsize++;
            start = end;
        }
        end++;
    }
    int dir_name_len = end-start-1;
    dir_name[dirsize] = (char*)calloc(dir_name_len+1,sizeof(char));
    strncpy(dir_name[dirsize],start+1,dir_name_len);
    dirsize++;
    start = end;
    char **ret = (char**)malloc(sizeof(char*)*dirsize);
    memcpy(ret,dir_name,sizeof(char*)*dirsize);
    *size = dirsize;
    return ret;
}
//获取一个路径中最后的文件名。
char *get_file_name(const char *local_path)
{
    int local_path_len = strlen(local_path);
    char temp[1024] = {0};
    int i;
    for(i=0;i<local_path_len;++i)
    {
        if(local_path[local_path_len-1-i]=='/')
        {
            break;
        }
        temp[i] = local_path[local_path_len-1-i];
    }
    reverseString(temp);
    char *ret = (char*)calloc(i+1,sizeof(char));
    strcpy(ret,temp);
    return ret;
}
void get_user_path(const char *argument,token_info *token,char *user_path)
{
    if(strlen(argument)==0)
    {
        strcpy(user_path,token->workpath);
        return;
    }
    int path_num = 0;
    char **res = splite_file_name(token->workpath,&path_num);
    if(argument[0]=='/')
    {
        sprintf(user_path,"/%s%s",res[0],argument);
    }
    else
    {
        sprintf(user_path,"%s/%s",token->workpath,argument);
    }
    for(int i=0;i<path_num;++i)
    {
        free(res[i]);
    }
    free(res);
    char *temp = simplifyPath(user_path);
    memset(user_path,0,1024);
    strcpy(user_path,temp);
    free(temp);
}

