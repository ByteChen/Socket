#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define ERR_EXIT(m) do{ perror(m); exit(EXIT_FAILURE);} while(0)

struct packet{
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

void do_service(int sock) {
	struct packet pSendBuf;
	struct packet pReadBuf;
	memset(&pSendBuf, 0, sizeof(pSendBuf));
	memset(&pReadBuf, 0, sizeof(pReadBuf));

	while((fgets(pReadBuf.buf, sizeof(pReadBuf.buf), stdin)) > 0){
		int len = strlen(pReadBuf.buf);
		pReadBuf.len = htonl(len);
		// writen(sock, &pReadBuf.len, sizeof(int));
		writen(sock, &pReadBuf, sizeof(int)+len);

		// write(sock, rdbuf, strlen(rdbuf));
		//read(sock, wrbuf, sizeof(wrbuf));
		int ret = readn(sock, &pSendBuf.len, sizeof(int));
		if (ret == -1) {
			ERR_EXIT("readn");
		} else if(ret < 4) {
			printf("peer closed\n");
			break;
		}
		readn(sock, pSendBuf.buf, ntohl(pSendBuf.len));
		fputs(pSendBuf.buf, stdout);

		memset(&pSendBuf, 0, sizeof(pSendBuf));
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
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("connect");
	
	do_service(sock);

	close(sock);
	return 0;
}
