//增加子进程处理多个请求

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define ERR_EXIT(m) do{ perror(m); exit(EXIT_FAILURE);} while(0)

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

	if(bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");
	if(listen(sock, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	int connfd;
	struct sockaddr_in peer;
	socklen_t peerlen = sizeof(peer);
	while(1){
		if((connfd = accept(sock, (struct sockaddr*)&peer, &peerlen)) < 0)
			ERR_EXIT("accept");
		printf("peer connect: ip = %s, port = %d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
		int pid = fork();
		if(pid<0)
			ERR_EXIT("fork");
		else if(pid==0){
			close(sock);	//子进程不需要监听，所以关闭被动套接字 
			//处理连接细节
			char buf[1024];
			memset(buf, 0, sizeof(buf));
			while(1){
				int len = read(connfd, buf, sizeof(buf));
				//if(len <= 0)	//若保留这个判断，则client断开时，server会自动断开
				//	break;
				if(len == 0){ //client 退出
					printf("client close\n");
					break;
				}
				fputs(buf,stdout);
				write(connfd, buf, len);
				memset(buf, 0, len+1);
			}
			//client 退出后，子进程就没有存在的价值了，所以一定要手动关闭，
			//不然他会回到循环开头处，利用accept（）处理新连接。这显然是不合法的。
			exit(EXIT_SUCCESS);
		}
		else{
			close(connfd);	//parent 进程不需要这个连接，所以关闭
		}
	}
	close(sock);
	return 0;
}
