#include "header.h"

int log_fd[5] = {-1, -1, -1, -1, -1};
int log_isolate = -1;
int log_thread = -1;
char curr_date[11];
int output_level = -1;
int update_log_info(const char *now_date, int level);
void print_log(int level, const char *format, ...);
void get_now_date(char *date);
int get_log_fd(int level);
int str_to_lever(char *set_lever);

int get_log_isolate()
{
  if (log_isolate != -1)
  {
    return log_isolate;
  }
  printf("init log isolate\n");
  // LOG(LOG_LEVEL_DEBUG, "init log isolate");
  char *isloate = read_config("log_output_isolate");
  if (isloate == NULL)
  {
    printf("log_output_isolate is not set, use default value: 0\n");
    // LOG(LOG_LEVEL_WARNING, "log_output_isolate is not set, use default value: 0");
    log_isolate = 0;
  }
  else
  {
    printf("log_output_isolate: %s\n", isloate);
    log_isolate = strcmp(isloate, "true") == 0 ? 1 : 0;
    printf("log_isolate: %d\n", log_isolate);
    // LOG(LOG_LEVEL_DEBUG, "log_output_isolate: %s", isloate);
    free(isloate);
  }
  return log_isolate;
}

int get_curr_fd(int level)
{
  int is_isloate = get_log_isolate();
  int log_idx = 0;
  if (is_isloate == 1)
    log_idx = level + 1;
  return log_fd[log_idx];
}

int get_log_fd(int level)
{
  // 获取当前日期
  char now_date[11];
  get_now_date(now_date);
  int curr_fd = get_curr_fd(level);
  // 初始化文件描述符
  if (curr_fd == -1 || (strncmp(curr_date, now_date, 10) != 0))
  {
    // 更新 log_fd 和 curr_date
    curr_fd = update_log_info(now_date, level);
    return curr_fd;
  }

  return curr_fd;
}

void get_now_date(char *date)
{
  // 获取当前时间
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  // 提取年份、月份和日期
  int year = timeinfo->tm_year + 1900; // 年份从 1900 开始
  int month = timeinfo->tm_mon + 1;    // 月份从 0 开始
  int day = timeinfo->tm_mday;

  sprintf(date, "%d-%02d-%02d", year, month, day); // 格式化日期
}

int update_log_info(const char *now_date, int level)
{
  int curr_log_fd = get_curr_fd(level);
  if (curr_log_fd != -1)
  {
    close(curr_log_fd);
  }
  char log_path[128] = {0};

  // 获取是否分离日志
  int is_isloate = get_log_isolate();

  // 获取日志文件路径
  if (is_isloate == 1)
  {
    char level_str[20] = {0};
    level_to_str(level, level_str);
    sprintf(log_path, "%s/%s_%s.log", LOG_PATH, now_date, level_str);
  }
  else
    sprintf(log_path, "%s/%s.log", LOG_PATH, now_date);
  curr_log_fd = open(log_path, O_CREAT | O_RDWR | O_APPEND, 0666);

  // 更新本地缓存的文件描述符和日期
  if (is_isloate == 1)
  {
    log_fd[level + 1] = curr_log_fd;
  }
  else
  {
    log_fd[0] = curr_log_fd;
  }

  // 更新日期
  strcpy(curr_date, now_date);
  return curr_log_fd;
}

int get_thread_conf()
{
  if (log_thread != -1)
  {
    return log_thread;
  }
  char *thread = read_config("log_output_thread");
  if (thread == NULL)
  {
    printf("log_output_thread is not set, use default value: 0\n");
    log_thread = 0;
  }
  else
  {
    printf("配置文件读取到：%s\n", thread);
    log_thread = strcmp(thread, "true") == 0 ? 1 : 0;
    printf("配置文件读取到 log_thread:%d\n", log_thread);
    free(thread);
  }
  return log_thread;
}

void print_log(int level, const char *format, ...)
{
  va_list args;
  va_start(args, format); // 初始化参数列表
  char buf[1024] = {0};
  vsprintf(buf, format, args);
  va_end(args); // 结束参数列表

  if (get_thread_conf() == 1)
  {
    char buf_thread[20] = {0};
    sprintf(buf_thread, " [tid: %ld] ", pthread_self() % 10000);
    strcat(buf, buf_thread);
  }

  if (level == LOG_LEVEL_ERROR)
  {
    strcat(buf, "[errmsg:");
    strcat(buf, strerror(errno));
    strcat(buf, "]");
  }

  strcat(buf, "\n");
  int fd = get_log_fd(level);
  write(fd, buf, strlen(buf));
}

/// @brief 日志输出级别控制
/// @param level  日志界别
/// @return true 允许输出， false 不允许输出
bool output_control(int level)
{
  // 获取设置的日志层级
  if (output_level == -1)
  {
    char *level = read_config("log_output_level");
    if (level == NULL)
    {
      LOG(LOG_LEVEL_WARNING, "log_output_level is not set, use default value: INFO");
      output_level = LOG_LEVEL_INFO;
    }
    else
    {
      output_level = str_to_lever(level);
      if (output_level == -1)
      {
        LOG(LOG_LEVEL_WARNING, "log_output_level is not set, use default value: INFO");
        output_level = LOG_LEVEL_INFO;
      }
    }
  }
  return level >= output_level ? true : false;
}

/// @brief 字符串转换为日志等级
/// @param set_lever
/// @return 日志等级
int str_to_lever(char *set_lever)
{
  char *ch = set_lever;
  while (*ch != '\0')
  {
    *ch = toupper((*ch));
    ch++;
  }
  if (strcmp(set_lever, "DEBUG") == 0)
  {
    return LOG_LEVEL_DEBUG;
  }
  else if (strcmp(set_lever, "INFO") == 0)
  {
    return LOG_LEVEL_INFO;
  }
  else if (strcmp(set_lever, "WARNING") == 0)
  {
    return LOG_LEVEL_WARNING;
  }
  else if (strcmp(set_lever, "ERROR") == 0)
  {
    return LOG_LEVEL_ERROR;
  }
  else
  {
    return -1;
  }
}

/// @brief 日志等级转换为字符串
/// @param level 日志等级
/// @param level_str 日志等级字符串, 用于输出结果
/// @param int 转换成功 0， 否则 -1
int level_to_str(int level, char *level_str)
{
  if (level < 0 || level > 3)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "level is not valid for : %d", level);
    return -1;
  }
  static char *levels[] = {"DBUG", "INFO", "WARN", "ERRO"};

  strncpy(level_str, levels[level], strlen(levels[level]));
  return 0;
}
