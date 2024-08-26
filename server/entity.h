#include <time.h>
#include <stdbool.h> 

/*此头文件用于声明数据库中实体对应与C语言中的结构题*/

/// @brief 用户信息 
typedef struct
{
  unsigned long id;
  char name[30];
  char password[70];
  time_t delete_time;
} user_t;

/// @brief 文件信息
typedef struct
{
  unsigned long id;
  int type;
  char real_path[1024];
  char hash_value[255];
  bool complete;
  int quote;
  time_t delete_time;
} file_t;

/// @brief 用户与文件的对应关系
typedef struct
{
  unsigned long id;
  char name[128];
  unsigned long user_id;
  unsigned long file_id;
  unsigned long parent_id;
  time_t delete_time;
} user_file_t;