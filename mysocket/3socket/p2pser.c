//点对点聊天服务器
//使用信号+ kill() + signal() 实现父进程关闭时通知子进程关闭
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>

#define ERR_EXIT(m) do{ perror(m); exit(EXIT_FAILURE);} while(0)

void handle(int sig){
	printf("recieve a signal:%d\n", sig);
	printf("close current child process..\n");
	exit(EXIT_SUCCESS);
}

int main(){
	int sock;
	if((sock=socket(AF_INET, SOCK_STREAM,0))<0){
		ERR_EXIT("socket");
	}

	struct sockaddr_in servaddr;
	memset(&servaddr,0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int on = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");

	if(bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");
	if(listen(sock, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	int connfd;
	struct sockaddr_in peer;
	socklen_t peerlen = sizeof(peer);
	if((connfd = accept(sock, (struct sockaddr*)&peer, &peerlen)) < 0)
		ERR_EXIT("accept");
	printf("peer connect: ip = %s, port = %d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));

	int pid = fork();
	if(pid < 0)
		ERR_EXIT("fork");
	else if(pid==0){		//子进程从键盘获取输入
		signal(SIGUSR1, handle);
		char buf[1024];
		memset(buf, 0, sizeof(buf));
		while(fgets(buf, sizeof(buf), stdin) != NULL){
			write(connfd,buf, strlen(buf));
			memset(buf, 0, strlen(buf)+1);
		}
		//结束输入后，进程退出
		printf("stop input, child close.\n");
		exit(EXIT_SUCCESS);
	}
	else{		//父进程从socket接收并打印
		char revbuf[1024] = {0};
		while(1){
			int nread = read(connfd, revbuf, sizeof(revbuf));
			if(nread == -1)
				ERR_EXIT("read");
			else if(nread == 0){
				printf("peer close\n");
				break;
			}
			else{
				fputs(revbuf, stdout);
			}
			memset(revbuf, 0, nread+1);
		}
		printf("parent close\n");
		kill(pid, SIGUSR1);
		exit(EXIT_SUCCESS);
	}
	close(sock);
	return 0;
}
