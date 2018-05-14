//p2p 聊天客户端

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
	printf("reacive a signal:%d\n", sig);
	printf("close current child process\n");
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
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("connect");
	
	char rdbuf[1024] = {0};
	char wrbuf[1024] = {0};
	int pid = fork();
	if(pid == -1)
		ERR_EXIT("fork");

	else if(pid == 0){
		signal(SIGUSR1,handle);
		while((fgets(rdbuf, sizeof(rdbuf), stdin)) != NULL){
			write(sock, rdbuf, strlen(rdbuf));
			memset(rdbuf, 0, strlen(rdbuf)+1);
		}
		printf("stop input, child close.\n");	//当键盘输入Ctrl+C时子进程结束
		exit(EXIT_SUCCESS);
	}
	else{
		while(1){
			int n = read(sock, wrbuf, sizeof(wrbuf));
			if(n == -1)
				ERR_EXIT("read");
			else if(n == 0){
				printf("peer close,");
				break;
			}
			fputs(wrbuf, stdout);
			memset(wrbuf, 0, strlen(wrbuf)+1);
		}
		printf("parent close\n");
		kill(pid, SIGUSR1);
		exit(EXIT_SUCCESS);
	}
	close(sock);
	return 0;
}
