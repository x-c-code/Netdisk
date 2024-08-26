#include "header.h"

int enqueue(TaskQueue *queue,int netfd,TransFile *train)
{
    if(queue->queue_size==queue->max_size)
    {
        //可以插入日志
        return -1;
    }
    Node *fd_node = (Node*)calloc(1,sizeof(Node));
    fd_node->netfd = netfd;
    memcpy(&fd_node->train,train,sizeof(TransFile));
    if(queue->queue_size==0)
    {
        queue->head = fd_node;
    }
    else
    {
        queue->tail->next= fd_node;
    }
    queue->tail = fd_node;
    queue->queue_size++;
    LOG(LOG_LEVEL_DEBUG,"insert command_no = %d",train->command_no);
    if(train->command_no!=5&&train->command_no!=6)
    {
        queue->short_command_size++;
    }
    return 0;
}

int dequeue(TaskQueue *queue ,TransFile *train)
{
    // 获取当前线程tid
    pthread_t self = pthread_self();
    if(queue->head==NULL)
    {
        return -1;
    }
    // 创建虚拟头
    Node temp;
    temp.next = queue->head;
    Node *prev = &temp;
    Node *cur = queue->head;
    LOG(LOG_LEVEL_DEBUG,"这是 %ld 线程,short_thread = %ld,queue->size = %d",self==short_thread,short_thread,queue->queue_size);

    while(cur!=NULL)
    {
        // 如果是长命令且线程是长命令线程则退出
        if(cur->train.command_no==5 || cur->train.command_no==6)
        {
            if(self!=short_thread)
            {
                break;
            }
        }
        // 如果是短命令且是短命令线程则退出
        else if(self == short_thread && cur->train.command_no!=5 && cur->train.command_no!=6)
        {
            break;
        }

        cur = cur->next;
        prev = prev->next;
    }
    Node *test_node = queue->head;
    while(test_node!=NULL)
    {
        LOG(LOG_LEVEL_DEBUG,"command_no = %d",test_node->train.command_no);
        test_node=test_node->next;
    }
    // 如果没有找到目标任务且是长命令线程，取头节点短线程执行
    if(cur == NULL&&self!=short_thread)
    {
        cur = queue->head;
        queue->head = queue->head->next;  
    }
    else if(cur !=NULL)
    {
        // 如果找到了目标任务，则更新任务队列
        prev->next= cur->next;
    }
    else if(cur == NULL && self == short_thread)
    {
        return -1;
    }
    if(cur!=NULL&&cur->next==NULL)
    {
        queue->tail=prev;
    }
    LOG(LOG_LEVEL_DEBUG,"self = %ld,command_no = %d",self,cur->train.command_no);
    cur->next = NULL;
    queue->queue_size--;
    memcpy(train,&cur->train,sizeof(TransFile));
    if(cur->train.command_no!=5&&cur->train.command_no!=6)
    {
        queue->short_command_size--;
    }
    int ret_fd = cur->netfd;
    free(cur);

    // if(queue->queue_size==1)
    // {
    //     queue->head = NULL;
    //     queue->tail = NULL;
    // }
    // else
    // {
    //     queue->head = queue->head->next;
    // }
    // queue->queue_size--;
    // int ret_fd = tmp->netfd;
    // free(tmp);
    return ret_fd;
}