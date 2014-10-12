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

void child_read(int data_fd);
void farther_write(int data_fd);

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
	pid_t pid = fork();
	if (pid < 0)
		ERR_EXIT("fork");
	else if (0 == pid) { //child read
		child_read(client_fd);
		close(client_fd);
		exit(EXIT_SUCCESS);
	}
	//farther write
	farther_write(client_fd);
	//farther close
	close(client_fd);

	return 0;
}
void child_read(int data_fd)
{
	//read display
	char recv_buf[1024] = {'\0'};
	while (1) {
		bzero(recv_buf, sizeof(recv_buf));
		int recv_len = read(data_fd, recv_buf, sizeof(recv_buf));
		if (0 == recv_len) {
			printf("client close\n");
			break;
		}
		else if (recv_len < 0)
			ERR_EXIT("server read");
		//success
		fputs(recv_buf, stdout);
	}
}

void farther_write(int data_fd)
{
	char send_buf[1024] = {'\0'};
	while (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
		write(data_fd, send_buf, strlen(send_buf));
		bzero(send_buf, sizeof(send_buf));
	}
}
