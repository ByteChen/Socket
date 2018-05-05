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
	if((connfd = accept(sock, (struct sockaddr*)&peer, &peerlen)) < 0)
		ERR_EXIT("accept");
	
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	while(1){
		int len = read(connfd, buf, sizeof(buf));
		if(len <= 0)
			break;
		fputs(buf,stdout);
		write(connfd, buf, len);
		memset(buf, 0, len+1);
	}
	close(connfd);
	return 0;
}
