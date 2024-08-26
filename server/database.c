#include "header.h"

typedef struct
{
  MYSQL *connection;
  bool in_use;
} Connection;

typedef struct
{
  Connection connects[MAX_POOL_SIZE];
  int available;
  int max_size;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} Db_pool;

Db_pool *pool;

/// @brief 初始化数据库连接池
void init_database_pool()
{
  // 从配置文件中读取数据库配置
  char *host = read_config("mysql_host");
  char *user = read_config("mysql_name");
  char *password = read_config("mysql_password");
  char *db = read_config("mysql_database");
  int port = atoi(read_config("mysql_port"));
  char *character_set = read_config("mysql_charset");
  LOG_WITH_TIME(LOG_LEVEL_INFO, "host: %s, user: %s, password: %s, db: %s, port: %d, character_set: %s",
                host, user, password, db, port, character_set);

  // 初始化数据库连接池对象
  pool = (Db_pool *)malloc(sizeof(Db_pool));
  ERROR_CHECK(pool, NULL, "malloc failed in init_pool for database");
  pthread_cond_init(&(pool->cond), NULL);
  pthread_mutex_init(&(pool->mutex), NULL);
  pool->available = MAX_POOL_SIZE;
  pool->max_size = MAX_POOL_SIZE;

  // 初始化数据库连接
  for (int i = 0; i < MAX_POOL_SIZE; ++i)
  {

    pool->connects[i].connection = mysql_init(NULL);
    if (!mysql_real_connect(pool->connects[i].connection, host, user, password, db, port, NULL, 0))
    {
      LOG_WITH_TIME(LOG_LEVEL_ERROR, "Failed to connect to MySQL: Error: %s\n", mysql_error(pool->connects[i].connection));
      exit(1);
    }
    // 设置字符集
    mysql_set_character_set(pool->connects[i].connection, character_set);
    pool->connects[i].in_use = false;
  }
  LOG_WITH_TIME(LOG_LEVEL_INFO, "Database connection pool initialized successfully");
}

/// @brief 获取数据库连接
/// @return 返回一个数据库连接, 禁止手动释放，请使用release_connection释放
MYSQL *get_connection()
{
  // 获取数据库连接， 没有可用连接则等待
  pthread_mutex_lock(&(pool->mutex));
  while (pool->available == 0)
  {
    pthread_cond_wait(&(pool->cond), &(pool->mutex));
  }

  for (int i = 0; i < MAX_POOL_SIZE; ++i)
  {
    if (!pool->connects[i].in_use)
    {
      pool->connects[i].in_use = true;
      pool->available--;
      pthread_mutex_unlock(&(pool->mutex));
      return pool->connects[i].connection;
    }
  }
  pthread_mutex_unlock(&(pool->mutex));
  return NULL;
}

/// @brief 释放数据库连接
/// @param connection 要释放的数据库连接
void release_connection(MYSQL *connection)
{
  pthread_mutex_lock(&(pool->mutex));
  for (int i = 0; i < MAX_POOL_SIZE; ++i)
  {
    if (pool->connects[i].connection == connection)
    {
      pool->connects[i].in_use = false;
      pool->available++;
      pthread_cond_signal(&(pool->cond));
      break;
    }
  }
  pthread_mutex_unlock(&(pool->mutex));
}

/// @brief 销毁数据库连接池
void destory_database_pool()
{
  // 等待所有数据库连接使用结束
  pthread_mutex_lock(&(pool->mutex));
  while (pool->available != MAX_POOL_SIZE)
  {
    pthread_cond_wait(&(pool->cond), &(pool->mutex));
  }

  // 释放数据库连接
  for (int i = 0; i < MAX_POOL_SIZE; ++i)
  {
    mysql_close(pool->connects[i].connection);
  }
  // 释放数据库连接池对象
  pthread_mutex_unlock(&(pool->mutex));
  pthread_mutex_destroy(&(pool->mutex));
  pthread_cond_destroy(&(pool->cond));
  free(pool);
  LOG_WITH_TIME(LOG_LEVEL_INFO, "Database connection pool cleaned up successfully");
}
/*    使用示例
 *    MYSQL *connection = get_connection();
 *
 *    if (connection)
 *    {
 *      query(connection, "select * from user where id < %d", 5);
 *      query_set *result_set = get_result_set(connection, convert_to_user, sizeof(user_t));
 *      for (int i = 0; i < result_set->size; i++)
 *      {
 *        user_t user = ((user_t *)(result_set->data))[i];
 *        printf("id: %lu, name: %s, password: %s, delete_time: %lu\n", user.id, user.name, user.password, user.delete_time);
 *      }
 *      free_result_set(result_set);
 *      release_connection(connection);
 *    }
 */

/// @brief 查询辅助函数
/// @param conn 数据库连接
/// @param format 查询语句格式串
/// @param ... 可变长的插入变量
/// @return 0表示成功，-1表示失败
int query(MYSQL *conn, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  char query[1024];
  vsnprintf(query, 1024, format, args);
  LOG_WITH_TIME(LOG_LEVEL_DEBUG, "Query SQL: %s\n", query);
  va_end(args);
  if (mysql_query(conn, query))
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "Query failed: %s\n", mysql_error(conn));
    return -1;
  }
  return 0;
}

int query_security(MYSQL *conn, const char *format, MYSQL_BIND *bind_params, int bind_params_size)
{
  MYSQL_STMT *stmt = mysql_stmt_init(conn);
  if (!stmt)
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "无法初始化MySQL语句对象: %s\n", mysql_error(conn));
    return -1;
  }

  if (mysql_stmt_prepare(stmt, format, strlen(format)))
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "无法准备查询：%s\n", mysql_error(conn));
    mysql_stmt_close(stmt);
    return -1;
  }

  if (mysql_stmt_bind_param(stmt, bind_params))
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "无法绑定参数：%s\n", mysql_error(conn));
    mysql_stmt_close(stmt);
    return -1;
  }

  LOG_WITH_TIME(LOG_LEVEL_DEBUG, "Query SQL: %s\n", format);
  for (int i = 0; i < bind_params_size; i++)
  {
    if (bind_params[i].buffer_type == MYSQL_TYPE_STRING)
      LOG_WITH_TIME(LOG_LEVEL_DEBUG, "Bind param %d: %s\n", i, (char *)bind_params[i].buffer);
    else if (bind_params[i].buffer_type == MYSQL_TYPE_DOUBLE)
      LOG_WITH_TIME(LOG_LEVEL_DEBUG, "Bind param %d: %f\n", i, *(double *)bind_params[i].buffer);
    else if (bind_params[i].buffer_type == MYSQL_TYPE_LONG)
      LOG_WITH_TIME(LOG_LEVEL_DEBUG, "Bind param %d: %d\n", i, *(int *)bind_params[i].buffer);
  }

  if (mysql_stmt_execute(stmt))
  {
    LOG_WITH_TIME(LOG_LEVEL_ERROR, "无法执行查询：%s\n", mysql_error(conn));
    mysql_stmt_close(stmt);
    return -1;
  }

  mysql_stmt_close(stmt);
  return 0;
}

/// @brief 获取查询结果集
/// @param conn 数据库连接
/// @param convert 目标对象转换函数
/// @param data_size 目标对象大小
/// @return 查询结果集
query_set *get_result_set(MYSQL *conn, void (*convert)(MYSQL_ROW *row, void *target), unsigned int data_size)
{
  MYSQL_RES *result = mysql_store_result(conn);
  int num_rows = mysql_num_rows(result);
  query_set *set = (query_set *)malloc(sizeof(query_set));
  set->size = num_rows;
  set->data = calloc(num_rows , data_size);
  int idx = 0;
  MYSQL_ROW row_curr;
  while ((row_curr = mysql_fetch_row(result)))
  {
    convert(&row_curr, (set->data) + idx * data_size);
    idx++;
  }
  mysql_free_result(result);
  return set;
}

/// @brief 释放查询结果集
void free_result_set(query_set *set)
{
  free(set->data);
  free(set);
}