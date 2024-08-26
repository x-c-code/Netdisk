#include "head.h"

//去除命令开头和结尾的空格
char *trim(char *str) {
    char *end;
    // 去除字符串开头的空格 
    while (isspace(*str))
    {
        str++;
    }
    // 如果字符串全是空格，则直接返回空字符串 
    if (*str == '\0')
    {
        return str;
    }
    // 去除字符串结尾的空格 
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
    {
        end--;                                                                                                  
    } // 在结尾加上 null 终止符 
    *(end + 1) = '\0';
    return str;
}

//把一段路径化为绝对路径
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
            // 如果中间间隔两位判断间隔位是不是两个'.'，如果是则nameidx跳转到上上>
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
//反转字符串
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

//获取路径最后一段
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
    temp[i] = '/';
    reverseString(temp);
    char *ret = (char*)calloc(i+1,sizeof(char));
    strcpy(ret,temp);
    return ret;
}
//获取配置信息
int getparameter(char *key, char *value){

   FILE * file = fopen("ipconfig.conf", "r");
   while(1){
       char line[100];
       bzero(line, sizeof(line));
       // 读一行数据
       char *res = fgets(line, sizeof(line), file);
       if(res == NULL){
           char buf[] = "没有要找的内容 \n";
           memcpy(value, buf, strlen(buf));
           return -1;
       }
       // 处理数据
       char *line_key = strtok(line, "=");
       if(strcmp(key, line_key) == 0){
           // 要找的内容
           char *line_value = strtok(NULL, "=");
           memcpy(value, line_value, strlen(line_value));
           return 0;
       }
   }
   
   return 0;
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