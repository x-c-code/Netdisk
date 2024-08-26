#include "header.h"
#include "tool.h"

//login
query_set *get_user(const char *user_name)
{
    MYSQL *connect = get_connection();
    if(connect == NULL)
    {
        return NULL;
    }
    query(connect,"select * from user where name = '%s' ",user_name);
    query_set *result_set = get_result_set(connect,convert_to_user,sizeof(user_t));
    if(result_set->size==0)
    {
        release_connection(connect);
        LOG(LOG_LEVEL_DEBUG,"没有这个用户");
        return NULL;
    }
    release_connection(connect);
    return result_set;
}

//register
int user_register(int client_fd,char *account,char *password)
{
    LOG(LOG_LEVEL_DEBUG,"register account: %s,password %s",account,password);
    MYSQL *connect = get_connection();
    //数据库连接错误
    if(connect == NULL)
    {
        return -1;
    }
    mysql_autocommit(connect,0);
    query(connect, "select * from user where name = '%s' ",account);
    query_set *result_set = get_result_set(connect,convert_to_user,sizeof(user_t));
    if(result_set->size!=0)
    {//用户已存在
        release_connection(connect);
        return 0;
    }
    char hash_word[CRYPT_OUTPUT_SIZE+1] = {0};
    encrypt_password(password,hash_word);
    
    query(connect, "insert into user (name,password) values ('%s', '%s') ",account,hash_word);
    unsigned long user_id = mysql_insert_id(connect);
    
    query(connect,"insert into file (type,complete,quote) values ('dir','1',1) ");
    unsigned long file_id = mysql_insert_id(connect);
    
    query(connect,"insert into user_file (name,user_id,file_id) values ('%s',%lu,%lu) ",account,user_id,file_id);
    mysql_commit(connect);
    mysql_autocommit(connect,1);

    release_connection(connect);
    
    return user_id;
}

//cd 查询传入的路径是否合法，合法返回最后一项的id，不合法则返回0或-1
// -1 数据库查询错误 0查询失败，非0合法
int query_cd(const char *path,unsigned long user_id)
{
    LOG_WITH_TIME(LOG_LEVEL_DEBUG,"query_cd: path = %s",path);
    MYSQL *connect = get_connection();
    if(connect==NULL)
    {
        return -1;
    }
    query(connect,
    "select * from user_file where user_id = %ld order by file_id ",
    user_id);
    query_set *file_set = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));
    if(file_set->size==0)
    {
        free_result_set(file_set);
        release_connection(connect);
        return -1;
    }
    user_file_t *file = (user_file_t*)(file_set->data);
    int path_depth = 0;
   
    char **path_name = splite_file_name(path,&path_depth);

    int parent_id = file[0].parent_id;
    int path_confirm = 0;
    for (int i=0;i<file_set->size;++i)
    {
        if(path_confirm==path_depth)
        {
            break;
        }
        LOG(LOG_LEVEL_DEBUG,"query_cd:path_name[path_comfirm] = %s",path_name[path_confirm]);
        if(strcmp(file[i].name,path_name[path_confirm])==0)
        {
            if(parent_id == file[i].parent_id)
            {
                parent_id = file[i].file_id;
                path_confirm++;
                continue;
            }
        }
    }

    for(int i=0;i<path_depth;++i)
    {
        free(path_name[i]);
    }
    free(path_name);
    free_result_set(file_set);
    release_connection(connect);
    LOG(LOG_LEVEL_DEBUG,"query_cd:path_comfirm = %d",path_confirm);
    return path_confirm==path_depth?parent_id:0;
}

//ls
//NULL 表示数据库错误，path为当前工作目录，file_num为传入传出参数，实际意义为当前目录下文件数量
query_set *query_ls(const char *path,unsigned long user_id)
{
    LOG_WITH_TIME(LOG_LEVEL_DEBUG,"query_ls: path = %s",path);
    int cur_dir_id = query_cd(path,user_id);
    if(cur_dir_id<=0)
    {
        LOG(LOG_LEVEL_DEBUG,"query_ls: query_cd <= 0");
        return NULL;
    }
    MYSQL *connect = get_connection();
    if(connect == NULL)
    {
        LOG(LOG_LEVEL_DEBUG,"query_ls: connect = NULL");
        return NULL;
    }
    //char *dir_name = get_file_name(path);
    query(connect,
    "select * from user_file where file_id in (select id from file where complete = '1') and parent_id = %ld order by file_id",
    cur_dir_id
    );
    query_set *result_set = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));

    release_connection(connect);
    return result_set;
}

//ll
query_set *query_ll(const char *path, unsigned long user_id)
{
    LOG_WITH_TIME(LOG_LEVEL_DEBUG,"query_ll: path = %s",path);
    int cur_dir_id = query_cd(path,user_id);
    if(cur_dir_id<=0)
    {
        LOG(LOG_LEVEL_DEBUG,"query_ll: query_cd = NULL");
        return NULL;
    }
    MYSQL *connect = get_connection();
    if(connect == NULL)
    {
        LOG(LOG_LEVEL_DEBUG,"query_ll: connect = NULL");
        return NULL;
    }

    query(connect,"select * from file where id in (select file_id from user_file where parent_id = %ld) and complete = '1' order by id",cur_dir_id);
    query_set *result_set = get_result_set(connect,convert_to_file,sizeof(file_t));

    release_connection(connect);
    // free_result_set(result_set);
    return result_set;
}
//pwd

//mkdir
//-1数据库连接错误，0 路径非法，1 同名文件，2成功
int query_mkdir(const char *path, char *dir_name,unsigned long user_id)
{
    LOG_WITH_TIME(LOG_LEVEL_DEBUG,"query_mkdir: path = %s",path);
    int cur_dir_id = query_cd(path,user_id);
    if(cur_dir_id<=0)
    {
        return cur_dir_id;
    }
    MYSQL *connect = get_connection();
    //查询当前目录下是否已有同名文件或文件夹，如果有则创建失败
    query(connect,
    "select * from user_file where parent_id = %lu and name = '%s' ",
    cur_dir_id,dir_name
    );
    query_set *query_res = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));
    //有同名文件
    if(query_res->size!=0)
    {
        free_result_set(query_res);
        release_connection(connect);
        return 1;
    }
    mysql_autocommit(connect,0);
    //在文件表中插入一个文件夹项
    query(connect, "insert into file(type,complete,quote) values ('dir','1',1) ");
    int dir_id = mysql_insert_id(connect);
    //char *cur_dir_name = get_file_name(path);
    query(connect,"insert into user_file(name,user_id,file_id,parent_id) values ('%s',%lu,%lu,%lu) ",dir_name,user_id,dir_id,cur_dir_id);
    mysql_commit(connect);
    mysql_autocommit(connect,1);
    release_connection(connect);
    return 2;
}
//puts
//在传入的path目录中添加文件，
//判断目录合法性，并返回当前目录的文件id，
//服务器错误  -2
//路径非法  -1
//同名 0
//未完整 1
//完整 2
// 检查file是否有这个哈希值，如果没有则直接新建开始传输
//如果有检查是否完成，完成则检查同目录下是否有同名文件或文件夹，如果有则报错，没有则建立连接
//如果未完成则检查当前用户同文件夹下是否有同名文件，如果没有则直接开始进行续传，如果有也报错
int query_puts(const char *path, char *file_name,char *hash_value,unsigned long user_id)
{
    int cur_dir_id = query_cd(path,user_id);
    if(cur_dir_id < 0)
    {
        return -2;
    }
    if(cur_dir_id == 0)
    {
        return -1;
    }
    MYSQL *connect = get_connection();
    //查找同目录下是否有同名文件，如果有检查是否是同一份文件
    query(connect,
    "select * from user_file where parent_id = %lu and name = '%s' ",
    cur_dir_id,file_name
    );
    query_set *query_res = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));
    //有同名文件
    if(query_res->size!=0)
    {
        int same_name_id = ((user_file_t*)query_res->data)[0].file_id;
        free_result_set(query_res);
        query(connect,
        "select * from file where id = %ld",
        same_name_id
        );
        query_res = get_result_set(connect,convert_to_file,sizeof(file_t));
        //查找同名文件的哈希值,如果二者相等则说明是同一份文件，直接进行续传
        file_t *same_name_file = (file_t*)(query_res->data);
        int ret = 0;
        if(strcmp(hash_value,same_name_file->hash_value)==0)
        {
            ret = same_name_file->complete?2:1;
        }
        free_result_set(query_res);
        release_connection(connect);
        return ret;
    }
    free_result_set(query_res);
    //没有同名文件则查找文件库是否有这个文件
    query(connect
    ,"select * from file where hash = '%s'"
    ,hash_value
    );
    query_res = get_result_set(connect,convert_to_file,sizeof(file_t));
    if(query_res->size==0)
    {//表示没有这个文件
        mysql_autocommit(connect,0);
        query(connect
        ,"insert into file(type,real_path,hash,complete,quote) values ('file','%s','%s','0',1) "
        ,hash_value,hash_value);
        unsigned long file_id = mysql_insert_id(connect);
        query(connect
        ,"insert into user_file (name,user_id,file_id,parent_id) values ('%s',%lu,%lu,%lu)"
        ,file_name,user_id,file_id,cur_dir_id);
        free_result_set(query_res);
        mysql_commit(connect);
        mysql_autocommit(connect,1);
        release_connection(connect);
        return 1;
    }
    //有这个文件，则查看是否完整，如果完整就秒传，没有就续传
    file_t *file_info = (file_t*)(query_res->data);
    bool is_complete = file_info->complete;
    
    query(connect
    ,"insert into user_file (name,user_id,file_id,parent_id) values ('%s',%lu,%lu,%lu)"
    ,file_name,user_id,file_info->id,cur_dir_id);
    release_connection(connect);
    free_result_set(query_res);
    return is_complete?2:1;
}
//gets
//在传入的path目录中寻找文件，判断哈希值是否存在且完成，如果存在且完成将哈希值存入hash_value返回1, 可以开始进行传输
//如果存在但未完成以及不存在则返回0。
//路径非法则返回-1
//如果服务器错误返回-2；
int query_gets(const char *path, char *file_name,char *hash_value,unsigned long user_id)
{
    int cur_dir_id = query_cd(path,user_id);
    LOG(LOG_LEVEL_DEBUG,"QUERY_GETS: path = %s",path);
    //服务器错误
    if(cur_dir_id<0)
    {
        return -1;
    }
    //路径错误
    if(cur_dir_id==0)
    {
        return 0;
    }
    //路径合法开始查找
    MYSQL *connect = get_connection();
    query(connect,"select * from file where id in (select file_id from user_file where user_id = %lu and parent_id = %lu and name = '%s' ) and type = 'file'"
    ,user_id,cur_dir_id,file_name
    );
    query_set * query_res=get_result_set(connect,convert_to_file,sizeof(file_t));
    file_t *file_info = (file_t*)(query_res->data);
    //文件不存在或者标记为未完成
    if(query_res->size==0||file_info->complete==0)
    {
        LOG(LOG_LEVEL_DEBUG,"query_res->data = %d,",query_res->size);
        free_result_set(query_res);
        release_connection(connect);
        return 0;
    }
    LOG(LOG_LEVEL_DEBUG,"file_info->hash_value = %s",file_info->hash_value);
    strcpy(hash_value,file_info->hash_value);
    LOG(LOG_LEVEL_DEBUG,"query_gets: input hash_value = %s",hash_value);
    free_result_set(query_res);
    release_connection(connect);
    return 1;
}

//更新上传的文件完成度为1
int query_update(const char *path,char *hash_value, unsigned long user_id)
{
    MYSQL *connect = get_connection();
    if(connect==NULL)
    {
        return 1;
    }
    query(connect,"update file set complete = '1' where hash = '%s' ",hash_value);
    release_connection(connect);
    return 0;
}




//rm
//-1数据库查询错误，0 删除成功 1 找不到文件，2 文件夹非空
int query_remove(const char *path, char *file_name,unsigned long user_id)
{
    LOG_WITH_TIME(LOG_LEVEL_DEBUG,"query_remove: path = %s",path);
    int cur_dir_id = query_cd(path,user_id);
    if(cur_dir_id <=0)
    {
        return cur_dir_id;
    }
    MYSQL *connect = get_connection();
    //查找user_file 中目标目录下名字与传入目标相同的表项
    query(connect,
     "select * from user_file where file_id in (select file_id from user_file where user_id = %lu and parent_id = %lu ) and name like '%s' "
     ,user_id,cur_dir_id,file_name);
    query_set *res_userfile = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));
    if(res_userfile->size==0)
    {//说明目标目录下没有所选文件
        free_result_set(res_userfile);
        release_connection(connect);
        return 1;
    }
    user_file_t *user_file_info = (user_file_t*)res_userfile->data;
    LOG(LOG_LEVEL_DEBUG,"user_file[0].id = %lu,name = %s,user_id = %lu,file_id = %lu,parent_id = %lu"
    ,user_file_info[0].id,user_file_info[0].name,user_file_info[0].user_id,user_file_info[0].file_id,user_file_info[0].parent_id);

    unsigned long file_id = user_file_info[0].file_id;
    free_result_set(res_userfile);
    //查找file表中id为所需删除的项
    query(connect, "select * from file where id = %lu ",file_id);
    query_set *res_file = get_result_set(connect,convert_to_file,sizeof(file_t));
    file_t *file_info = (file_t*)res_file->data;
    LOG(LOG_LEVEL_DEBUG,"file_info.id = %lu,type = %d,real_path = %s, hash_value = %s, complete = %d, quote = %d",
    file_info->id,file_info->type,file_info->real_path,file_info->hash_value,file_info->complete,file_info->quote);
    //这是一个文件
    if(file_info->type==1)
    {
        //去掉userfile中与file的关联
        query(connect,"delete from user_file where file_id = %lu and user_id = %lu ",file_id,user_id);
    }
    else if (file_info->type==0)
    {//这是一个文件夹
        //查找是否有文件或者文件夹的父id为需要删除项
        query(connect,"select * from user_file where parent_id = %lu ",file_id);
        query_set *res_child_file = get_result_set(connect,convert_to_userfile,sizeof(user_file_t));
        LOG(LOG_LEVEL_DEBUG,"res_child_file->size = %d",res_child_file->size);
        if(res_child_file->size!=0)
        {//如果有说明目录非空，返回报错
            LOG(LOG_LEVEL_DEBUG,"目录非空");
            free_result_set(res_child_file);
            free_result_set(res_file);
            release_connection(connect);
            return 2;
        }
        //没有就直接删除
        free(res_child_file);
        query(connect,"delete from user_file where file_id = %lu and user_id = %lu ",file_id,user_id);
    }
    free_result_set(res_file);
    release_connection(connect);
    return 0;
}