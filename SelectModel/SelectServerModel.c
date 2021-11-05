#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


int main(void)
{
	//1，创建用于监听的socket文件描述符sfd
	int sfd = socket(AF_INET, SOCK_STREAM,0);
	if(sfd == -1)
	{
		perror("socket");
		exit(0);
	}
	//2，绑定本地IP和端口到文件描述符
	
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(8080);
	myaddr.sin_addr.s_addr = INADDR_ANY;
	//myaddr.sin_addr.s_addr = inet_addr("101.34.203.98");
	int res = bind(sfd,(struct sockaddr*)&myaddr,sizeof(myaddr));
	if(res == -1)
	{
		perror("bind");
		printf("28行\n");
		exit(0);
	}
	//3，让套接字处于监听状态
	res = listen(sfd,128);
	if(res == -1)
	{
		perror("listed");
		printf("36行\n");
		exit(0);
	}

	int cfd = accept(sfd,NULL,NULL);
	if(cfd == -1)
	{
		perror("accept");
		exit(0);
	}
	else
	{
		printf("客户端请求成功！\n");
	}
	while(1)
	{
		char SendBuff[1024];
		char RecvBuff[1024];
		memset(SendBuff,0,sizeof(SendBuff));
		
		printf("我自己:");
		scanf("%s",SendBuff);

		write(cfd,SendBuff,strlen(SendBuff));
		int len = read(cfd,RecvBuff,sizeof(RecvBuff));
		if(len > 0)
		{
			printf("客户端say: %s\n",RecvBuff);
			
			//scanf("%s",SendBuff);         
			//write(cfd,SendBuff,strlen(SendBuff)+1);
		}
		else if(len == 0)
		{
			printf("客户端端口连接\n");
			break;
		}
		else
		{
			perror("read");
			break;
		}
	}

	close(sfd);
	close(cfd);
}


















