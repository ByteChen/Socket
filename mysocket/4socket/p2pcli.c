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

// p2p 聊天客户端

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
	printf("reacive a signal:%d\n", sig);
	printf("close current child process\n");
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
	stServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(iSockFd, (struct sockaddr*)&stServAddr, sizeof(stServAddr)) < 0)
		ERR_EXIT("connect");

	char szRdBuf[1024] = {0};
	char szWrBuf[1024] = {0};
	int pid = fork();
	if (pid == -1) {
		ERR_EXIT("fork");
    } else if (pid == 0) {
		signal(SIGUSR1,handle);
		while ((fgets(szRdBuf, sizeof(szRdBuf), stdin)) != NULL) {
			write(iSockFd, szRdBuf, strlen(szRdBuf));
			memset(szRdBuf, 0, strlen(szRdBuf)+1);
		}
		printf("stop input, child close.\n");	// 当键盘输入Ctrl+C时子进程结束
		exit(EXIT_SUCCESS);
	} else {
		while(1) {
			int n = read(iSockFd, szWrBuf, sizeof(szWrBuf));
			if (n == -1) {
				ERR_EXIT("read");
            } else if (n == 0) {
				printf("peer close,");
				break;
			}
			fputs(szWrBuf, stdout);
			memset(szWrBuf, 0, strlen(szWrBuf)+1);
		}
		printf("parent close\n");
		kill(pid, SIGUSR1);
		exit(EXIT_SUCCESS);
	}
	close(iSockFd);
	return 0;
}
