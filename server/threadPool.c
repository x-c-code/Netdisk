#include "header.h"



/// @brief 初始化线程池
/// @param num 线程数量 , taskQueue的最大长度为3*num
/// @param pipe_in 线程池与主线程通信的管道, 往管道写
/// @return PthreadPool* 线程池指针， 出错程序直接退出
PthreadPool *init_thread_pool(int num, int pipe_in)
{
    PthreadPool *pool = (PthreadPool *)calloc(1, sizeof(PthreadPool));
    ERROR_CHECK(pool, NULL, "calloc failed");
    pool->thread_id = (pthread_t *)calloc(num, sizeof(pthread_t));
    if (pool->thread_id == NULL)
    {
        free(pool);
        LOG_WITH_TIME(LOG_LEVEL_ERROR, "calloc failed");
        exit(-1);
    }
    pool->num = num;
    pool->q.short_command_size=0;
    pool->q.max_size = 3 * num;
    pool->pipe_in = pipe_in;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    for (int i = 0; i < num; ++i)
    {
        pthread_create(&pool->thread_id[i], NULL, thread, pool);
        LOG_WITH_TIME(LOG_LEVEL_INFO, "thread [%lu] create", pool->thread_id[i]);
    }
    short_thread = pool->thread_id[0];
    pool->exit = false;
    return pool;
}

/// @brief 销毁线程池
/// @param pool 目标线程池指针
void destroy_thread_pool(PthreadPool *pool)
{
    for (int i = 0; i < pool->num; ++i)
    {
        pthread_t tid = pool->thread_id[i];
        pthread_join(pool->thread_id[i], NULL);
        LOG_WITH_TIME(LOG_LEVEL_INFO, "thread [%lu] join success", tid);
    }
    free(pool->thread_id);
    free(pool);
    LOG_WITH_TIME(LOG_LEVEL_INFO, "thread pool destroy sucess");
}