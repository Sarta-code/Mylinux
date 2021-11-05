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
    else
    {
	    printf("服务器连接成功！\n");
    }

    //和客户端通信
    while(1)
    {
        //数据缓冲区
        char SendBuff[1024];
	char RecvBuff[1024];
        memset(SendBuff,0,sizeof(SendBuff));
	memset(RecvBuff,0,sizeof(RecvBuff));
        
	//scanf("%s",SendBuff);
	//write(cfd,SendBuff,strlen(SendBuff)+1);

	int len = read(cfd,RecvBuff,sizeof(RecvBuff));
        if(len > 0)
        {
            printf("服务器say: %s\n",RecvBuff);
	    printf("我自己:");
	    scanf("%s",SendBuff);
	    write(cfd,SendBuff,strlen(SendBuff)+1);
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
    }

    close(cfd);
    
    return 0;
}
