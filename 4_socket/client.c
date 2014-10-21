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

struct packet
{
	int len;
	char buf[1024];
};

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);

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
	struct packet send_packet, recv_packet;
	bzero(&send_packet, sizeof(send_packet));
	bzero(&recv_packet, sizeof(recv_packet));
	int send_len = 0;
	int recv_len = 0;
	while (fgets(send_packet.buf, sizeof(send_packet.buf), stdin) != NULL) {
		send_len = strlen(send_packet.buf);
		send_packet.len = htonl(send_len);
		writen(client_fd, &send_packet, 4 + send_len);

		int ret = readn(client_fd, &recv_packet.len, 4);
		if (-1 == ret)
			ERR_EXIT("read");
		else if (ret < 4)
		{
			printf("client close\n");
			break;
		}
		recv_len = ntohl(recv_packet.len);
		ret = readn(client_fd, recv_packet.buf, recv_len);
		if (-1 == ret)
			ERR_EXIT("read");
		else if (ret == 0)
		{
			printf("client close\n");
			break;
		}
		fputs(recv_packet.buf, stdout);
		bzero(&send_packet, sizeof(send_packet));
		bzero(&recv_packet, sizeof(recv_packet));
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
