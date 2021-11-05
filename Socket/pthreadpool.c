#include "pthreadpool.h"
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#define true 1
#define false 0
const int NUMBER = 2;
//任务队列结构体
typedef struct Task
{
	void (*function)(void* arg);
	void* arg;
}Task;

//线程池结构体
struct pthreadpool
{
	//任务队列
	Task* TaskQ;
	int QueueCapacity;	//容量
	int Queuesize;	//当前任务个数
	int QueueFront;	//对头
	int QueueRear;	//队尾

	pthread_t managerID;	//管理者线程
	pthread_t* pthreadIDs;	//工作现场
	int minNum;	//最先线程数
	int maxNum;	//最大线程数
	int busyNum; //忙的线程数
	int liveNum; //存活线程数
	int exitNum; //要销毁线程数
	pthread_mutex_t mutexPool;	//锁定整个线程池
	pthread_mutex_t mutexBusy;	//锁定busyNum变量
	pthread_cond_t notFull;	//任务队列是不是满了
	pthread_cond_t notEmpty;	//任务队列是不是空了

	int shotdown;	//是不是要销毁线程池，销毁为1，不销毁为0
};

ThreadPool* ThreadPoolCreate(int min, int max, int QueueSize)
{
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	do
	{
		if (pool == NULL)
		{
			printf("pool Memory allocation failure\n");
			break;
		}
		pool->pthreadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->pthreadIDs == NULL)
		{
			printf("work pthread allocation failure\n");
			break;
		}
		memset(pool->pthreadIDs, 0, sizeof(pthread_t) * max);
		pool->maxNum = max;
		pool->minNum = min;
		pool->busyNum = 0;
		pool->liveNum = min;
		pool->exitNum = 0;

		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0)
		{
			printf("Lock and condition variables failed to initialize\n");
			break;
		}
		//任务队列
		pool->TaskQ = (Task*)malloc(sizeof(Task) * QueueSize);
		pool->QueueCapacity = QueueSize;
		pool->Queuesize = 0;
		pool->QueueFront = 0;
		pool->QueueRear = 0;

		pool->shotdown = false;

		//创建线程
		pthread_create(&pool->managerID, NULL, manager, (void*)pool);//管理者线程
		for (int i = 0; i < min; i++)
		{
			pthread_create(&pool->pthreadIDs[i], NULL, worker, (void*)pool);
		}
		return pool;
	} while (0);

	if (pool->pthreadIDs != NULL)free(pool->pthreadIDs);
	if (pool->TaskQ != NULL)free(pool->TaskQ);
	if (pool != NULL)free(pool);
	return NULL;
}
int threadPoolDestroy(ThreadPool* pool)
{
	if (pool == NULL)
	{
		return -1;
	}
	//关闭线程池
	pool->shotdown = true;
	
	//阻塞回收管理者线程
	pthread_join(pool->managerID, NULL);

	//唤醒阻塞的消费者线程
	for (int i = 0;i < pool->liveNum;i++)
	{
		pthread_cond_signal(&pool->notEmpty);
		//pthread_cond_broadcast(&pool->notEmpty);
	}
	//释放malloc的堆内存
	if (pool->TaskQ)
	{
		free(pool->TaskQ);
		pool->TaskQ = NULL;
	}
	
	//释放互斥量和条件变量
	pthread_mutex_destroy(&pool->mutexBusy);
	pthread_mutex_destroy(&pool->mutexPool);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
    if(pool->pthreadIDs)
    {
        free(pool->pthreadIDs);
        pool->pthreadIDs = NULL;
    }
    
	free(pool);
	pool = NULL;
	return 0;
}
void* worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;

	while (true)
	{
		pthread_mutex_lock(&pool->mutexPool);
		//当前任务是否为空
		while (pool->Queuesize == 0 && pool->shotdown == false)
		{
			//阻塞工作线程
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			//判断是不是要销毁线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexPool);
					threadExit(pool);
				}	
			}
		}
		
		//判断线程是否被关闭
		if (pool->shotdown == true)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			threadExit(pool);
		}
		//从任务队列里取出任务
		Task task;
		task.function = pool->TaskQ[pool->QueueFront].function;
		task.arg = pool->TaskQ[pool->QueueFront].arg;
		//移动队头
		pool->QueueFront = (pool->QueueFront + 1) % pool->QueueCapacity;
		pool->Queuesize--;
		//解锁
		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);

		printf("pthread %ld start work\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexBusy);

		task.function(task.arg);
		free(task.arg);
		task.arg = NULL;


		printf("pthread %ld end work\n", pthread_self());
		pthread_mutex_lock(&pool->mutexBusy);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexBusy);
	}
	return NULL;
}
void threadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	while (pool->Queuesize == pool->QueueCapacity && !pool->shotdown)
	{
		//阻塞生产者线程
		pthread_cond_wait(&pool->notFull,&pool->mutexPool);
	}
	if (pool->shotdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	//添加任务
	pool->TaskQ[pool->QueueRear].function = fun;
	pool->TaskQ[pool->QueueRear].arg = arg;
	pool->QueueRear = (pool->QueueRear + 1) % pool->QueueCapacity;
	pool->Queuesize++;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexBusy);
	int BusyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusy);
	return BusyNum;
}
int threadPoolAliveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexPool);
	int liveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return liveNum;
}

void* manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (pool->shotdown == 0)
	{
		//每隔三秒检测一次
		sleep(3);

		//取出线程池中任务的个数和当前线程的个数
		pthread_mutex_lock(&pool->mutexPool);
		int QueueSize = pool->Queuesize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		//取出忙的线程个数
		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

		//添加线程
		//添加条件:任务数 > 当前线程个数&&当前线程个数 < 最大线程数
		if (QueueSize > (liveNum - busyNum) && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int counter = 0;
			for (int i = 0;i < pool->maxNum && counter < NUMBER
				&& pool->liveNum < pool->maxNum;i++)
			{
				if (pool->pthreadIDs[i] == 0)
				{
					pthread_create(&pool->pthreadIDs[i], NULL, worker, (void*)pool);
					counter++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}


		//销毁线程
		//销毁条件:忙的线程*2 < 存活的线程 && 存活的线程 > 最小线程
		if (busyNum*2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);

			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return NULL;
}

void threadExit(ThreadPool* pool)
{
	if (pool == NULL)
	{
		return;
	}
	pthread_t pthID = pthread_self();
	for (int i = 0;i < pool->maxNum;i++)
	{
		if (pool->pthreadIDs[i] == pthID)
		{
			pool->pthreadIDs[i] = 0;
			printf("pthread %ld Exit\n", pthID);
			break;
		}
		
	}
	pthread_exit(NULL);
}

