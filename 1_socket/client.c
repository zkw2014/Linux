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
#include <strings.h>

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)


//socket connect fgets write read close
int main(int argc, const char *argv[])
{
	//socket
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == client_fd)
		ERR_EXIT("client socket");

	//connect
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8989);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (-1 == connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
		ERR_EXIT("client connect");

	//communication
	char send_buf[1024] = {'\0'};
	char recv_buf[1024] = {'\0'};
	while (fgets(send_buf, 1024, stdin) != NULL) {
		write(client_fd, send_buf, 1024);
		read(client_fd, recv_buf, 1024);
		fputs(recv_buf, stdout);
		bzero(send_buf, 1024);
		bzero(recv_buf, 1024);
	}

	//close
	close(client_fd);

	return 0;
}
