#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "pthreadpool.h"

void working(void*arg);
void acceptconn(void*arg);
struct sockinfo
{
   struct sockaddr_in addr;
   int fd;
};
typedef struct poolinfo
{
    ThreadPool* p;
    int fd;
}poolinfo;

int main(void)
{
    //1创建用于监听的套接字，这个套接字是一个文件描述
    int fd = socket(AF_INET,SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        exit(0);
    }

    //将得到的套接字文件描述符和本地IP和端口进行绑定
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(8080);
    my_addr.sin_addr .s_addr = INADDR_ANY;
    //my_addr.sin_addr.s_addr = inet_addr("172.17.0.13");
    int ret = bind(fd,(struct sockaddr*)&my_addr,sizeof(my_addr));
    if(ret == -1)
    {
        perror("bind");
        exit(0);
    }

    //设置监听（监听客户端的连接）
    ret = listen(fd,128);
    if(ret == -1)
    {
        perror("listen");
        exit(0);
    }
    //创建线程池
    ThreadPool*pool = ThreadPoolCreate(3,10,100);
    //添加任务回调函数参数
    poolinfo *info = (poolinfo*)malloc(sizeof(poolinfo));
    info->p = pool;
    info->fd = fd;
    //添加任务
    threadPoolAdd(pool,acceptconn,info);
    pthread_exit(NULL);

    return 0;
}
void acceptconn(void*arg)
{
    poolinfo *p = (poolinfo*)arg;
    //等待客户端的连接请求，建立新的连接，会得到一个新的文件描述符，没有连接就阻塞
    int clilen = sizeof(struct sockaddr_in);
    while(1)
    {
        struct sockinfo* caddr;
        caddr = (struct sockinfo*)malloc(sizeof(struct sockinfo));
        
        caddr->fd = accept(p->fd,(struct sockaddr*)&caddr->addr,&clilen);
        if(caddr->fd == -1)
        {
            perror("accept");
            exit(0);
        }
         //添加通信任务
         threadPoolAdd(p->p,working,caddr);
         
    }

    close(p->fd);
}

void working(void*arg)
{
    struct sockinfo* caddr = (struct sockinfo*)arg;
    //打印客户端信息
    char clidata[24] = {0};
    printf("客户端IP:%s,端口:%d\n", inet_ntop(AF_INET, &caddr->addr.sin_addr.s_addr, clidata, sizeof(clidata)),ntohs(caddr->addr.sin_port));
   
    //和客户端通信
    while(1)
    {

        //接受客户端数据的缓冲区
        char buff[1024];
        memset(buff,0,sizeof(buff));
        int len = read(caddr->fd,buff,sizeof(buff));
        if(len > 0)
        {
            printf("客户端say: %s\n",buff);
            write(caddr->fd,buff,len);
        }
        else if(len == 0)
        {
            printf("客户端断开连接！\n");
            break;
        }
        else
        {
            perror("read");
            break;
        }
    }

    close(caddr->fd);
}
