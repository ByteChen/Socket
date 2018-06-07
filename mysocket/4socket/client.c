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

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>

#define ERR_EXIT(m) do { perror(m); exit(EXIT_FAILURE);} while (0)

struct packet{
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

void do_service(int sock) {
	struct packet pSendPacket;
	struct packet pReadPacket;
	memset(&pSendPacket, 0, sizeof(pSendPacket));
	memset(&pReadPacket, 0, sizeof(pReadPacket));
	/*
	while ((fgets(pReadPacket.buf, sizeof(pReadPacket.buf), stdin)) != NULL) {
		int len = strlen(pReadPacket.buf);
		pReadPacket.len = htonl(len);
		// writen(sock, &pReadPacket.len, sizeof(int));
		writen(sock, &pReadPacket, sizeof(int)+len);

		// write(sock, rdbuf, strlen(rdbuf));
		// read(sock, wrbuf, sizeof(wrbuf));
		int ret = readn(sock, &pSendPacket.len, sizeof(int));
		if (ret == -1) {
			ERR_EXIT("readn");
		} else if (ret < 4) {
			printf("peer closed.\n");
			break;
		}
		readn(sock,pSendPacket.buf, ntohl(pSendPacket.len));
		fputs(pSendPacket.buf, stdout);

		memset(&pSendPacket, 0, sizeof(pSendPacket));
		memset(&pReadPacket, 0, sizeof(pReadPacket));
	}*/
	int iStdinFd = fileno(stdin);
	int iMaxFd = iStdinFd > sock ? iStdinFd : sock;
	fd_set setFd;
	FD_ZERO(&setFd);
	while (1) {
		FD_SET(iStdinFd, &setFd);
		FD_SET(sock,&setFd);
		int iReady = select(iMaxFd+1, &setFd, NULL, NULL, NULL);
		if (iReady == -1) {
			ERR_EXIT("select");
		}
		if (FD_ISSET(iStdinFd, &setFd)) {
			if ((fgets(pReadPacket.buf, sizeof(pReadPacket.buf), stdin)) == NULL) {
				printf("close current connect.\n");
				break;
			}
			int len = strlen(pReadPacket.buf);
			pReadPacket.len = htonl(len);
			writen(sock, &pReadPacket, sizeof(int)+len);
			memset(&pReadPacket, 0, sizeof(pReadPacket));
		}
		if (FD_ISSET(sock, &setFd)) {
			int iRead = readn(sock, &pSendPacket.len, sizeof(int));
			if (iRead == -1 ) {
				ERR_EXIT("readn");
			} else if (iRead < sizeof(int)) {
				printf("peer close.\n");
				break;
			}
			readn(sock,pSendPacket.buf, ntohl(pSendPacket.len));
			fputs(pSendPacket.buf, stdout);
			memset(&pSendPacket, 0, sizeof(pSendPacket));
		}
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
	stServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(iSockFd, (struct sockaddr*)&stServAddr, sizeof(stServAddr)) < 0)
		ERR_EXIT("connect");
	
	do_service(iSockFd);

	close(iSockFd);
	return 0;
}
