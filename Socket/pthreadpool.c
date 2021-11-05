#include "pthreadpool.h"
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#define true 1
#define false 0
const int NUMBER = 2;
//������нṹ��
typedef struct Task
{
	void (*function)(void* arg);
	void* arg;
}Task;

//�̳߳ؽṹ��
struct pthreadpool
{
	//�������
	Task* TaskQ;
	int QueueCapacity;	//����
	int Queuesize;	//��ǰ�������
	int QueueFront;	//��ͷ
	int QueueRear;	//��β

	pthread_t managerID;	//�������߳�
	pthread_t* pthreadIDs;	//�����ֳ�
	int minNum;	//�����߳���
	int maxNum;	//����߳���
	int busyNum; //æ���߳���
	int liveNum; //����߳���
	int exitNum; //Ҫ�����߳���
	pthread_mutex_t mutexPool;	//���������̳߳�
	pthread_mutex_t mutexBusy;	//����busyNum����
	pthread_cond_t notFull;	//��������ǲ�������
	pthread_cond_t notEmpty;	//��������ǲ��ǿ���

	int shotdown;	//�ǲ���Ҫ�����̳߳أ�����Ϊ1��������Ϊ0
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
		//�������
		pool->TaskQ = (Task*)malloc(sizeof(Task) * QueueSize);
		pool->QueueCapacity = QueueSize;
		pool->Queuesize = 0;
		pool->QueueFront = 0;
		pool->QueueRear = 0;

		pool->shotdown = false;

		//�����߳�
		pthread_create(&pool->managerID, NULL, manager, (void*)pool);//�������߳�
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
	//�ر��̳߳�
	pool->shotdown = true;
	
	//�������չ������߳�
	pthread_join(pool->managerID, NULL);

	//�����������������߳�
	for (int i = 0;i < pool->liveNum;i++)
	{
		pthread_cond_signal(&pool->notEmpty);
		//pthread_cond_broadcast(&pool->notEmpty);
	}
	//�ͷ�malloc�Ķ��ڴ�
	if (pool->TaskQ)
	{
		free(pool->TaskQ);
		pool->TaskQ = NULL;
	}
	
	//�ͷŻ���������������
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
		//��ǰ�����Ƿ�Ϊ��
		while (pool->Queuesize == 0 && pool->shotdown == false)
		{
			//���������߳�
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			//�ж��ǲ���Ҫ�����߳�
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
		
		//�ж��߳��Ƿ񱻹ر�
		if (pool->shotdown == true)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			threadExit(pool);
		}
		//�����������ȡ������
		Task task;
		task.function = pool->TaskQ[pool->QueueFront].function;
		task.arg = pool->TaskQ[pool->QueueFront].arg;
		//�ƶ���ͷ
		pool->QueueFront = (pool->QueueFront + 1) % pool->QueueCapacity;
		pool->Queuesize--;
		//����
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
		//�����������߳�
		pthread_cond_wait(&pool->notFull,&pool->mutexPool);
	}
	if (pool->shotdown)
	{
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	//�������
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
		//ÿ��������һ��
		sleep(3);

		//ȡ���̳߳�������ĸ����͵�ǰ�̵߳ĸ���
		pthread_mutex_lock(&pool->mutexPool);
		int QueueSize = pool->Queuesize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);

		//ȡ��æ���̸߳���
		pthread_mutex_lock(&pool->mutexBusy);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusy);

		//����߳�
		//�������:������ > ��ǰ�̸߳���&&��ǰ�̸߳��� < ����߳���
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


		//�����߳�
		//��������:æ���߳�*2 < �����߳� && �����߳� > ��С�߳�
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

