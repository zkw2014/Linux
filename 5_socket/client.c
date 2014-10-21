/**************************************************************
	> file: client.c
	> by: zkw
 **************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t readline(int sockfd, void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);

//socket connect fgets write read close
int main(int argc, const char *argv[])
{
	//socket
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0)
		ERR_EXIT("client socket");

	//connect
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8989);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		ERR_EXIT("client connect");

	//communication
	char send_buf[1024] = {'\0'};
	char recv_buf[1024] = {'\0'};
	while (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
		writen(client_fd, send_buf, sizeof(send_buf));
		int ret = readline(client_fd, recv_buf, sizeof(recv_buf));
		if (ret < 0)
			ERR_EXIT("readline");
		else if (0 == ret) {
			printf("server close\n");
			break;
		}
		fputs(recv_buf, stdout);
		bzero(send_buf, sizeof(send_buf));
		bzero(recv_buf, sizeof(recv_buf));
	}

	//close
	close(client_fd);

	return 0;
}

//return value:
//value == n 
//value [0, n) 遇到EOF
//value < 0 错误
ssize_t readn(int fd, void *buf, size_t count)
{
	char *p = buf;
	size_t nleft = count;
	ssize_t nread = 0;

	while (nleft > 0) {
		nread = read(fd, p, nleft);
		if (nread < 0 && errno == EINTR) //中断时
			continue;
		else if (nread < 0) //其它不可恢复错误
			return -1;
		else if (nread == 0) //EOF
			return count - nleft;
		//读取到字符
		nleft -= nread; 
		p += nread;
	}

	return count - nleft; //value == n
}

//return value:
//value == n   success
//value <  0   error
ssize_t writen(int fd, const void *buf, size_t count)
{
	const char *p = buf;
	size_t nleft = count;
	ssize_t nwrite = 0;
	
	while (nleft > 0) {
		nwrite = write(fd, buf, nleft);
		if (nwrite < 0 && errno == EINTR)
			continue;
		else if (nwrite < 0)
			return -1;
		// >=0
		nleft -= nwrite;
		p += nwrite;
	}

	return count - nleft;
}

//先偷窥，然后再判断偷窥到的是否有\n;
//return value:
//value == -1 :   error
//value == 0  :   client close
//value > 0   :   ok
ssize_t readline(int sockfd, void *buf, size_t count)
{
	char *p = buf;
	size_t nleft = count;
	ssize_t nread = 0;

	while (nleft > 0) {
		nread = recv_peek(sockfd, p, nleft);
		if (nread < 0)
			return nread;
		else if (0 == nread) {
			return 0;
		}
		//偷窥到了数据,查看是否有\n
		int ix = 0;
		int ret = 0;
		for (ix = 0; ix != ret; ++ix) {
			if (p[ix] == '\n') {
				ret = readn(sockfd, p, ix + 1);
				if (ret != ix +1)
					ERR_EXIT("readn");
				return count;
			}
		}

		//偷窥到的数据中没有\n,先把数据全部取出，然后开始下一循环
		ret = readn(sockfd, p, nread);
		if (ret != nread)
			ERR_EXIT("readn");
		p += ret;
		nleft -= ret;
	}

	return count;
}

//return value:
// 0   client close
// -1  error
// >0  ok
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	int ret = 0;
	while ((ret = recv(sockfd, buf, len, MSG_PEEK)) < 0) {
		if (errno == EINTR)
			continue;
		else
			return -1;
	}

	return ret;
}
