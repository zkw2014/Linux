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
		write(client_fd, send_buf, strlen(send_buf));
		read(client_fd, recv_buf, sizeof(recv_buf));
		fputs(recv_buf, stdout);
		bzero(send_buf, sizeof(send_buf));
		bzero(recv_buf, sizeof(recv_buf));
	}

	//close
	close(client_fd);

	return 0;
}
