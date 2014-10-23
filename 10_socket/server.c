/**************************************************************
  > file: server.c
  > by: zkw
 **************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <pthread.h>

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

void echo_srv(int data_fd);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t readline(int sockfd, void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);

void handle_sigchld(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void *thread_func(void *arg)
{
	int *p = (int *)arg;
	int data_fd = *p;
	free(p);
	pthread_detach(pthread_self());
	echo_srv(data_fd);
	close(data_fd);

	return NULL;
}

//socket bind listen accept read write close
int main(int argc, const char *argv[])
{
	signal(SIGCHLD, handle_sigchld);
	signal(SIGPIPE, SIG_IGN);

	//socket
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		ERR_EXIT("server socket");

	//deal with timewait
	int on = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("server setsockopt");

	//bind
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8989);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		ERR_EXIT("server bind");

	//listen
	if (listen(listen_fd, SOMAXCONN) < 0)
		ERR_EXIT("server listen");

	//accept
	int data_fd;
	struct sockaddr_in peer_addr;
	socklen_t peer_addr_len = sizeof(peer_addr);

	while(1) {
		if ((data_fd = accept(listen_fd, (struct sockaddr *)&peer_addr, &peer_addr_len)) < 0)
			ERR_EXIT("server accept");
		printf("client's ip is :%s, port is :%d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

		pthread_t tid;
		int *p = (int *)malloc(sizeof(int)); //注意这里要用动态内存分配
		*p = data_fd;
		if (pthread_create(&tid, NULL, thread_func, p) != 0) {
			fprintf(stderr, "error:%s\n", "pthread_create");
			free(p);
			exit(EXIT_FAILURE);
		}
	}

	//close
	close(listen_fd);

	return 0;
}

void echo_srv(int data_fd)
{
	//read display write
	char recv_buf[1024] = {'\0'};
	while (1) {
		bzero(recv_buf, sizeof(recv_buf));
		int recv_len = readline(data_fd, recv_buf, sizeof(recv_buf));
		if (0 == recv_len) {
			printf("client close\n");
			break;
		}
		else if (recv_len < 0)
			ERR_EXIT("server read");
		//success
		fputs(recv_buf, stdout);
		writen(data_fd, recv_buf, recv_len);
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
