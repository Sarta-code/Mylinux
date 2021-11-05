#ifndef _PTHREADPOOL_H
#define _PTHREADPOOL_H
typedef struct pthreadpool ThreadPool;
//创建并初始化线程池
ThreadPool* ThreadPoolCreate(int min, int max, int QueueSize);
//销毁线程池
int threadPoolDestroy(ThreadPool* pool);

//给线程池添加任务
void threadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg);
//获取线程池中工作线程个数
int threadPoolBusyNum(ThreadPool* pool);
//获取线程池中存活线程个数
int threadPoolAliveNum(ThreadPool* pool);

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);
#endif