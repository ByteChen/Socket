//增加子进程处理多个请求

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define ERR_EXIT(m) do{ perror(m); exit(EXIT_FAILURE);} while(0)

struct packet {
	int len;
	char buf[1024];
};

size_t readn(int fd, void *buf, size_t count){
	int nleft = count;
	int nread = 0;
	char* bufp = (char*)buf;

	while(nleft > 0){
		int n = read(fd, bufp + nread, nleft);	// nelft 还是 nleft+1 呢？read 的返回值是否算上'\0'?
		if(n == -1){
			if(errno == EINTR)
				continue;
			return -1;	//read fail
		}
		else if(n == 0)	//EOF
			return nread;
		else {
			nleft -= n;
			nread += n;
		}
	}
	return count;	//success
}

ssize_t writen(int fd, const void *buf, size_t count){
	int nleft = count;
	int nwritten = 0;
	char* bufp = (char*)buf;

	while(nleft > 0){
		int n = write(fd, bufp + nwritten, nleft);	//nelft 还是 nleft+1 呢？read 的返回值是否算上'\0'?
		if(n == -1){
			if(errno == EINTR)
				continue;
			return -1;	// write fail
		}
		else if(n == 0)	// EOF
			continue;
		else {
			nleft -= n;
			nwritten += n;
		}
	}
	return count;	//success
}

void do_service(int connfd){
	struct packet pReadBuf;
	memset(&pReadBuf, 0, sizeof(pReadBuf));
	while(1){
		int len = readn(connfd, &pReadBuf.len, sizeof(int));
		if(len == -1){
			ERR_EXIT("read");
		}
		else if(len < sizeof(int)){ //client 退出
			printf("client close\n");
			break;
		}
		readn(connfd, pReadBuf.buf, ntohl(pReadBuf.len));
		fputs(pReadBuf.buf,stdout);
		
		writen(connfd, &pReadBuf.len, sizeof(int));
		writen(connfd, pReadBuf.buf, ntohl(pReadBuf.len));
		memset(&pReadBuf, 0, sizeof(pReadBuf));
	}	
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
			do_service(connfd);
			printf("finished service, about to exit the child process.\n");
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
