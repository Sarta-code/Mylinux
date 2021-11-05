#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>




int main(void)
{
    
    //创建一个用于通信的套接字
    int cfd = socket(AF_INET,SOCK_STREAM,0);
    if(cfd == -1)
    {
        perror("socket");
        exit(0);
    }

    //连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    //addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_addr.s_addr = inet_addr("101.34.203.98");
    int ret  =  connect(cfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret == -1)
    {
        perror("connect");
        exit(0);
    }

    //和客户端通信
    int number = 0;
    while(1)
    {
        number++;
        //发送数据
        char buff[1024];
        sprintf(buff,"你好服务器...%d\n",number);
        write(cfd,buff,strlen(buff)+1);

        //接受数据
        memset(buff,0,sizeof(buff));
        int len = read(cfd,buff,sizeof(buff));
        if(len > 0)
        {
            printf("服务器say: %s\n",buff);
        }
        else if(len == 0)
        {
            printf("服务器断开连接\n");
            exit(0);
        }
        else
        {
            perror("read");
            exit(0);
        }
        sleep(1);
    }

    close(cfd);
    
    return 0;
}
