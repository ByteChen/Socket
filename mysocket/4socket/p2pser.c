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
// 1.点对点聊天服务器
// 2.使用信号+ kill() + signal() 实现父进程关闭时通知子进程关闭,防止出现僵尸进程

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>

#define ERR_EXIT(m) do { perror(m); exit(EXIT_FAILURE);} while (0)

void handle(int sig) {
	printf("recieve a signal:%d\n", sig);
	printf("close current child process..\n");
	exit(EXIT_SUCCESS);
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

	int iOperation = 1;
	if (setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, &iOperation, sizeof(iOperation)) < 0)
		ERR_EXIT("setsockopt");

	if (bind(iSockFd, (struct sockaddr*)&stServAddr, sizeof(stServAddr)) < 0)
		ERR_EXIT("bind");
	if (listen(iSockFd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	int iConnFd;
	struct sockaddr_in stPeer;
	socklen_t iPeerLen = sizeof(stPeer);
	if ((iConnFd = accept(iSockFd, (struct sockaddr*)&stPeer, &iPeerLen)) < 0)
		ERR_EXIT("accept");
	printf("peer connect: ip = %s, port = %d\n",
        inet_ntoa(stPeer.sin_addr), ntohs(stPeer.sin_port));

	int pid = fork();
	if (pid < 0) {
		ERR_EXIT("fork");
    } else if (pid==0) {		// 子进程从键盘获取输入
		signal(SIGUSR1, handle);
		char szBuffer[1024];
		memset(szBuffer, 0, sizeof(szBuffer));
		while (fgets(szBuffer, sizeof(szBuffer), stdin) != NULL) {
			write(iConnFd,szBuffer, strlen(szBuffer));
			memset(szBuffer, 0, strlen(szBuffer)+1);
		}
		// 结束输入后，进程退出
		printf("stop input, child close.\n");
		exit(EXIT_SUCCESS);
	} else {	// 父进程从socket接收并打印
		char revbuf[1024] = {0};
		while(1) {
			int nread = read(iConnFd, revbuf, sizeof(revbuf));
			if (nread == -1) {
				ERR_EXIT("read");
            } else if (nread == 0) {
				printf("peer close\n");
				break;
			} else {
				fputs(revbuf, stdout);
			}
			memset(revbuf, 0, nread+1);
		}
		printf("parent close\n");
		kill(pid, SIGUSR1);
		exit(EXIT_SUCCESS);
	}
	close(iSockFd);
	return 0;
}
