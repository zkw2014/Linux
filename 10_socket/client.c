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
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/epoll.h>

#define EPOLL_SIZE 1024
#define MAX_EVENTS 30

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t readline(int sockfd, void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);
void echo_cli(int client_fd);

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

	echo_cli(client_fd);

	return 0;
}

void echo_cli(int client_fd)
{
	//1.建立句柄
	int epoll_fd = epoll_create(EPOLL_SIZE);
	if (-1 == epoll_fd)
		ERR_EXIT("epoll_create");
	struct epoll_event event_arr[MAX_EVENTS], ev;
	bzero(event_arr, sizeof(event_arr));
	bzero(&ev, sizeof(ev));
	int stdin_fd = fileno(stdin);

	//注册两个监听符
	ev.events = EPOLLIN;
	ev.data.fd = stdin_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stdin_fd, &ev) < 0)
		ERR_EXIT("epoll_ctl");
	ev.events = EPOLLIN;
	ev.data.fd = client_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
		ERR_EXIT("epoll_ctl");

	int readn = 0;
	char send_buf[1024] = {'\0'};
	char recv_buf[1024] = {'\0'};
	while (1) {
		bzero(send_buf, sizeof(send_buf));
		bzero(recv_buf, sizeof(recv_buf));

		//进行监听
		readn = epoll_wait(epoll_fd, event_arr, MAX_EVENTS, -1);
		if (readn < 0 && errno == EINTR)
			continue;
		else if (readn < 0)
			ERR_EXIT("epoll_wait");
		else if (0 == readn)
			continue;

		//3.分别进行判断
		int ix = 0;
		for (; ix != readn; ++ix) {
			int fd = event_arr[ix].data.fd;
			if (fd == stdin_fd) {
				if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
					shutdown(client_fd, SHUT_WR); //关闭写端，以后也不再监听标准输入，等待服务器端传完数据后，结束client.
					ev.events = EPOLLIN;
					ev.data.fd = stdin_fd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, stdin_fd, &ev) < 0)
						ERR_EXIT("epoll_ctl");
				}
				writen(client_fd, send_buf, sizeof(send_buf));
			}

			if (fd == client_fd) {
				int ret = readline(client_fd, recv_buf, sizeof(recv_buf));
				if (0 == ret) {
					printf("server close\n");
					//清空句柄
					ev.events = EPOLLIN;
					ev.data.fd = client_fd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev) < 0)
						ERR_EXIT("epoll_ctl");
					ev.events = EPOLLIN;
					ev.data.fd = stdin_fd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, stdin_fd, &ev) < 0)
						ERR_EXIT("epoll_ctl");
					close(client_fd);
					return;
				} else if (ret < 0)
					ERR_EXIT("readline");
				//读取到数据
				printf("%s", recv_buf);
			}
		}
	}
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
