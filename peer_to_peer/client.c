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
#include <signal.h>

#define ERR_EXIT(m)\
	do {\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

void child_write(int data_fd);
void farther_read(int data_fd);
void handler(int signum);

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
	else if (0 == pid) { //child write
		signal(SIGUSR1, handler);
		child_write(client_fd);
		close(client_fd);
		printf("child close\n");
		exit(EXIT_SUCCESS);
	}
	//farther read
	farther_read(client_fd);
	//farther close
	printf("farther close\n");
	close(client_fd);
	kill(pid, SIGUSR1);

	return 0;
}
void farther_read(int data_fd)
{
	//read display
	char recv_buf[1024] = {'\0'};
	while (1) {
		bzero(recv_buf, sizeof(recv_buf));
		int recv_len = read(data_fd, recv_buf, sizeof(recv_buf));
		if (0 == recv_len) {
			printf("server close\n");
			break;
		}
		else if (recv_len < 0)
			ERR_EXIT("server read");
		//success
		fputs(recv_buf, stdout);
	}
}

void child_write(int data_fd)
{
	char send_buf[1024] = {'\0'};
	while (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
		write(data_fd, send_buf, strlen(send_buf));
		bzero(send_buf, sizeof(send_buf));
	}
}

void handler(int signum)
{
	printf("child close\n");
	exit(EXIT_SUCCESS);
}
