#include "header.h"

void convert_to_user(MYSQL_ROW *row, void *user)
{

  MYSQL_ROW row_curr = *row;
  user_t *u = (user_t *)user;
  u->id = strtoul(row_curr[0], NULL, 10);
  // printf("user id: %lu\n", u->id);
  strcpy(u->name, row_curr[1]);
  // printf("user name: %s\n", u->name);
  strcpy(u->password, row_curr[2]);
  // printf("user password: %s\n", u->password);
  // printf("delete time: %s\n", row_curr[3]);
  if (row_curr[3] == NULL)
  {
    return;
  }
  u->delete_time = strtoul(row_curr[3], NULL, 10);
}

void convert_to_file(MYSQL_ROW *row,void *file)
{
  MYSQL_ROW row_curr = *row;
  file_t *f = (file_t *)file;
  f->id = strtoul(row_curr[0], NULL, 10);
  f->type = strcmp("dir",row_curr[1])!=0;
  if(row_curr[2]!=NULL)
  {
    strcpy(f->real_path,row_curr[2]);
  }
  if(row_curr[3]!=NULL)
  {
    strcpy(f->hash_value,row_curr[3]);
  }
  f->complete = atoi(row_curr[4]);
  f->quote = atoi(row_curr[5]);
  if (row_curr[6] != NULL)
  {
    f->delete_time = strtoul(row_curr[6], NULL, 10);
  }
}

void convert_to_userfile(MYSQL_ROW *row,void *user_file)
{
  MYSQL_ROW row_curr = *row;
  user_file_t *uf = (user_file_t *)user_file;
  uf->id = strtoul(row_curr[0], NULL, 10);
  strcpy(uf->name,row_curr[1]);
  uf->user_id = strtoul(row_curr[2],NULL,10);
  uf->file_id = strtoul(row_curr[3],NULL,10);
  uf->parent_id = row_curr[4]==NULL?0:strtoul(row_curr[4],NULL,10);
  if (row_curr[5] != NULL)
  {
    uf->delete_time = strtoul(row_curr[5], NULL, 10);
  }
}

/// @brief  转换时间字符串（%d-%d-%d %d:%d:%d） 到MYSQL_TIME类型
/// @param time_str
/// @param timestamp
/// @return
int convert_to_time(char *time_str, MYSQL_TIME *timestamp)
{
  int ret = sscanf(time_str, "%d-%d-%d %d:%d:%d",
                   &timestamp->year, &timestamp->month, &timestamp->day,
                   &timestamp->hour, &timestamp->minute, &timestamp->second);
  if (ret < 0)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "convert time string to MYSQL_TIME failed");
    return -1;
  }
  return ret;
}

/// @brief 转换MYSQL_TIME到字符串
/// @param timestamp MYSQL_TIME对象
/// @param time_str 结果字符串
/// @return
int convert_to_str(MYSQL_TIME *timestamp, char *time_str)
{
  int ret = sprintf(time_str, "%d-%d-%d %d:%d:%d", timestamp->year, timestamp->month, timestamp->day,
                    timestamp->hour, timestamp->minute, timestamp->second);
  if (ret < -1)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "convert MYSQL_TIME to string failed");
    return -1;
  }
  return ret;
}