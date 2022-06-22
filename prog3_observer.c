#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#define BUFSIZE 1024

int valid_username = 0;

//send and recieve for username	
void message_handler(int i, int sockfd)
{
	char send_buf[BUFSIZE];
	char recv_buf[BUFSIZE];
	int nbyte_recvd;
	
	if (i == 0){
		fgets(send_buf, BUFSIZE, stdin);
		if (strcmp(send_buf , "/quit\n") == 0) {
			exit(0);
		}
		else{
			send(sockfd, send_buf, strlen(send_buf), 0);
		}
	}else {
		//check if server disconnected
		nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
		if(nbyte_recvd > 2000){
			printf("server disconnected\n");
			exit(0);
		}
		recv_buf[nbyte_recvd] = '\0';
		printf("%s\n" , recv_buf);
		fflush(stdout);
		if(recv_buf[0] == 'N'){
			printf("invalid username\n");
			exit(0);
		}
	}
}
		
void connect_init(int *sockfd, struct sockaddr_in *server_addr,char *address, int port)
{
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(port);
	server_addr->sin_addr.s_addr = inet_addr(address);
	memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);
	
	if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
}
	
int main(int argc, char **argv)
{
	if (argc != 3)
    	{
        fprintf(stderr, "Error: Wrong number of arguments\n");
        fprintf(stderr, "usage:\n");
        fprintf(stderr, "./participant address port\n");
        exit(EXIT_FAILURE);
	}

	int sockfd, fdmax, i;
	struct sockaddr_in server_addr;
	fd_set master;
	fd_set read_fds;
	
	connect_init(&sockfd, &server_addr, argv[1], atoi(argv[2]));
	FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(0, &master);
    FD_SET(sockfd, &master);
	fdmax = sockfd;
	if (!valid_username){
		printf("Enter a username\n");
	}
	while(1){
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		for(i=0; i <= fdmax; i++ ){
			if(FD_ISSET(i, &read_fds)){
				message_handler(i, sockfd);
			}
		}
		valid_username = 1;
	}
	printf("client-quited\n");
	close(sockfd);
	return 0;
}
