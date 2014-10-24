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
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

#define MAX_DATA_FD_SIZE 1024

void do_select_srv(int listen_fd);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t readline(int sockfd, void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);

void handle_sigchld(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
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

	do_select_srv(listen_fd);

	close(listen_fd);
	return 0;
}

//第一步：建立一个集合，把listen_fd添加进去
//第二步：while(1), 然后进行监听
//第三步：对监听到的事件分别进行处理
void do_select_srv(int listen_fd)
{
	//第一步：初始化和准备工作
	fd_set all_fds, read_fds;
	FD_ZERO(&all_fds);
	FD_ZERO(&read_fds);
	FD_SET(listen_fd, &all_fds);
	int maxfd = listen_fd;
	int arr_client_fd[MAX_DATA_FD_SIZE];
	int ix = 0;
	for (; ix != MAX_DATA_FD_SIZE; ++ix)
		arr_client_fd[ix] = -1;
	int max_ix = -1;

	while (1) {
		read_fds = all_fds; //每次循环都要更新read_fds

		//第二步：监听
		int nread = select(maxfd+1, &read_fds, NULL, NULL, NULL);
		if (nread < 0 && errno == EINTR)
			continue;
		else if (nread < 0)
			ERR_EXIT("select");
		else if (0 == nread) //select超时
			continue;

		//第三步：对监听到的事件分别进行处理
		// 1.处理连接事件
		if (FD_ISSET(listen_fd, &read_fds)) {
			//接受一个连接
			struct sockaddr_in peer_addr;
			bzero(&peer_addr, sizeof(peer_addr));
			socklen_t peer_addr_len = sizeof(peer_addr);
			int client_fd;
			if ((client_fd = accept(listen_fd, (struct sockaddr *)&peer_addr, &peer_addr_len)) < 0)
				ERR_EXIT("server accept");
			printf("client's ip is :%s, port is :%d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
			//善后处理：1）把连接放到数组里，更新maxi； 2）把连接放到集合里，更新maxfd;
			int iy = 0;
			for(; iy != MAX_DATA_FD_SIZE; ++iy) { //看数组里是否有空余位置
				if (arr_client_fd[iy] == -1) {
					arr_client_fd[iy] = client_fd;
					FD_SET(client_fd, &all_fds);
					if (maxfd < client_fd) maxfd = client_fd;
					if (max_ix < iy) max_ix = iy;
					break;
				}
			}
			//当数组满时，特殊处理
			if (iy == MAX_DATA_FD_SIZE)
				printf("too many clients, try to connection later\n");
			--nread;
			if (nread <= 0)
				continue;
		}

		// 2.处理数据通信事件
		int iz = 0;
		for (; iz <= max_ix; ++iz) {
			int data_fd = arr_client_fd[iz];
			if (data_fd == -1) 
				continue;
			if (FD_ISSET(data_fd, &read_fds)) {
				//和client进行communication
				char recv_buf[1024] = {'\0'};
				int recv_len = readline(data_fd, recv_buf, sizeof(recv_buf));
				//如果client关闭,善后处理：更新数组和集合
				if (recv_len == 0) {
					printf("client close\n");
					arr_client_fd[iz] = -1;
					FD_CLR(data_fd, &all_fds);
					close(data_fd);
				}
				else if (recv_len < 0)
					ERR_EXIT("server read");
				//success
				fputs(recv_buf, stdout);
				writen(data_fd, recv_buf, recv_len);
				--nread;
				if (nread <= 0)
					break;
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
