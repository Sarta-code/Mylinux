#ifndef _PTHREADPOOL_H
#define _PTHREADPOOL_H
typedef struct pthreadpool ThreadPool;
//��������ʼ���̳߳�
ThreadPool* ThreadPoolCreate(int min, int max, int QueueSize);
//�����̳߳�
int threadPoolDestroy(ThreadPool* pool);

//���̳߳��������
void threadPoolAdd(ThreadPool* pool, void(*fun)(void*), void* arg);
//��ȡ�̳߳��й����̸߳���
int threadPoolBusyNum(ThreadPool* pool);
//��ȡ�̳߳��д���̸߳���
int threadPoolAliveNum(ThreadPool* pool);

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);
#endif