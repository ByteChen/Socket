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
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("connect");
	
	char rdbuf[1024] = {0};
	char wrbuf[1024] = {0};
	while((fgets(rdbuf, sizeof(rdbuf), stdin)) > 0){
		write(sock, rdbuf, strlen(rdbuf));
		read(sock, wrbuf, sizeof(wrbuf));
		fputs(wrbuf, stdout);

		memset(wrbuf, 0, strlen(wrbuf)+1);
		memset(rdbuf, 0, strlen(rdbuf)+1);
	}
	close(sock);
	return 0;
}
