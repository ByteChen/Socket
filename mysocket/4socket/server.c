/*
* Copyright 2018 ByteChen. All rights reserved.
*
* Licensed under the BSD 3-Clause License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     https://opensource.org/licenses/BSD-3-Clause
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

// 要点：
// 1.使用子进程，一个子进程处理一个client连接
// 2.增加 SO_REUSEADDR 选项，允许服务器重用端口，不必等到TIME_WAIT结束
// 3.定义了一个packet消息体结构，头部是消息的长度；
// 4.封装了readn() 和 write() 函数，结合packet消息体，解决TCP粘包问题。
// 5.客户端使用select优化IO，避免进程被fgets()阻塞，无法及时响应对端的关闭操作。

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define ERR_EXIT(m) do { perror(m); exit(EXIT_FAILURE);} while (0)

struct packet {
	int len;
	char buf[1024];
};

// 模仿read()接口，封装一个readn()，读取count指定长度的内容
size_t readn(int fd, void *buf, size_t count) {
	int nleft = count;
	int nread = 0;
	char* pszBuffer = (char*)buf;

	while (nleft > 0) {
		int n = read(fd, pszBuffer + nread, nleft);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
            }
			return -1;	// read fail
		} else if (n == 0) {    // EOF
			return nread;
        } else {
			nleft -= n;
			nread += n;
		}
	}
	return count;	// success
}

// 模仿write()接口，封装一个writen()，写count指定长度的内容
ssize_t writen(int fd, const void *buf, size_t count) {
	int nleft = count;
	int nwritten = 0;
	char* pszBuffer = (char*)buf;

	while (nleft > 0) {
		int n = write(fd, pszBuffer + nwritten, nleft);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
            }
			return -1;	// write fail
		} else if (n == 0) {	// EOF
			continue;
        }  else {
			nleft -= n;
			nwritten += n;
		}
	}
	return count;	// success
}

// 服务函数，从套接口接收，打印到标准输出中，然后再把相同内容发送回去给client
void do_service(int connfd) {
	struct packet pReadPacket;
	memset(&pReadPacket, 0, sizeof(pReadPacket));
	while(1) {
		int len = readn(connfd, &pReadPacket.len, sizeof(int));
		if (len == -1) {
			ERR_EXIT("read");
		} else if (len < sizeof(int)) {  // client 退出
			printf("client close\n");
			break;
		}
		readn(connfd, pReadPacket.buf, ntohl(pReadPacket.len));
		fputs(pReadPacket.buf,stdout);
		
		// writen(connfd, &pReadPacket.len, sizeof(int));
		// writen(connfd, pReadPacket.buf, ntohl(pReadPacket.len));
        // 可以一次性发送
        writen(connfd, &pReadPacket, sizeof(int)+ntohl(pReadPacket.len));
		memset(&pReadPacket, 0, sizeof(pReadPacket));
	}	
}

int main() {
	int iSockFd;
	if ((iSockFd=socket(AF_INET, SOCK_STREAM,0))<0) {
		ERR_EXIT("socket");
	}

	struct sockaddr_in stServAddr;
	memset(&stServAddr,0, sizeof(stServAddr));
	stServAddr.sin_family = AF_INET;
	stServAddr.sin_port = htons(5188);
	stServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // 设置服务器地址重复利用，不需要等待TIME_WAIT结束即可重用地址
	int iOperation = 1;
	if(setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, &iOperation, sizeof(iOperation)) < 0)
		ERR_EXIT("setsockopt");

	if(bind(iSockFd, (struct sockaddr*)&stServAddr, sizeof(stServAddr)) < 0)
		ERR_EXIT("bind");
	if(listen(iSockFd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	int iConnFd;
	struct sockaddr_in stPeer;
	socklen_t iPeerLength = sizeof(stPeer);
	/*while(1) {
		if((iConnFd = accept(iSockFd, (struct sockaddr*)&stPeer, &iPeerLength)) < 0)
			ERR_EXIT("accept");
		printf("peer connect: ip = %s, port = %d\n",
            inet_ntoa(stPeer.sin_addr), ntohs(stPeer.sin_port));
		int pid = fork();
		if (pid<0) {
			ERR_EXIT("fork");
        } else if (pid==0) {
			close(iSockFd);  // 子进程不需要监听，所以关闭被动套接字 
			// 处理连接细节
			do_service(iConnFd);
			printf("finished service, about to exit the child process.\n");
			// client 退出后，子进程就没有存在的价值了，所以一定要手动关闭，
			// 不然他会回到循环开头处，利用accept（）处理新连接。这显然是不合法的。
			exit(EXIT_SUCCESS);
		} else {
			close(iConnFd);	    // parent 进程不需要这个连接，所以关闭
		}
	}*/

	
	close(iSockFd);
	return 0;
}
