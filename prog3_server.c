#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
	
#define BUFSIZE 1024
#define MAXCLIENTS 255

typedef struct{
	int pSD;				//all values zeroed out on disconnect
	int oSD;
	char username[11];
	time_t last_input_time;
} client_t;

//client_t *clients;
client_t clients[MAXCLIENTS];
int lsdo;
int lsdp;

//returns 0 if invalid username and 1 if valid. role is 0 for participant and 1 for observer
int valid_username(char *username, int role){
	//participant
	int valid = 1;
	if(role == 0){
		for(int i = 0; i < strlen(username); i ++){
			//check for bad charachters
			if(username[i] < 0) valid = 0;
			else if(((username[i] > 122) || (username[i] < 65)) && username[i] != 10) valid = 0;
			else if(username[i] == 91) valid = 0;
			else if(username[i] == 92) valid = 0;
			else if(username[i] == 93) valid = 0;
			else if(username[i] == 94) valid = 0;
			else if(username[i] == 96) valid = 0;
		}
		for(int j = 0; j < MAXCLIENTS; j ++){
			if(strcmp(clients[j].username, username) == 0){
				valid = 0;
			}
		}
		return valid;
	}
}

//message sending that includes the username of the sender
void send_to_observers(int i, int nbytes_recvd, char *recv_buf, fd_set *master){
	for (int j = 0; j < MAXCLIENTS; j++){
		char * user;
		char message[BUFSIZE];
		if (FD_ISSET(j, master) && j != lsdo && j != lsdp && j != i){
			//find the username of the sender (i)
			for(int n = 0; n < MAXCLIENTS; n++){
				if (clients[n].pSD == i){
					user = clients[n].username;
					break;
				}
			}
			int user_len, recv_len;
			user_len = strlen(user);
			//get size of message in recv_buf
			for(int n = 0; n < BUFSIZE; n++){
				if (recv_buf[n] == '\n' || recv_buf[n] == '\0'){
					recv_len = n;
					break;
				}
			}
			//put message together
			char delim[2] = {':', ' '};
			char spaces[12] = {'>', ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',};
			memset(message, 0, BUFSIZE);
			strncat(message, spaces, 12 - user_len);
			strncat(message, user, user_len - 1);
			strncat(message, delim, 2);
			strncat(message, recv_buf, recv_len);
			//only send to user j if their username is registered
			for(int n = 0; n < MAXCLIENTS; n++){
				if (clients[n].oSD == j && clients[n].username[0] != '\0'){
					if (send(j, message, BUFSIZE, 0) == -1) {
						perror("send");
					}
					break;
				}
			}
		}
	}
	
}
/* Like send_to_all but does not include username
	used to broadcast message from server 
	Does not need to be called in a loop*/
void send_to_clients(int i, int nbytes_recvd, char *recv_buf, fd_set *master) {
	//iterate through all clients/observers
	//if fd is observer then send message
	int observer = 0;
	for (int j = 0; j < MAXCLIENTS; j++){
		observer = 0;
		//check if j has a registered username yet
		int registered_username = 0;
		int n = 0;
		for(n = 0; n < MAXCLIENTS; n++){
			if (clients[n].pSD == j || clients[n].oSD == j){
				if(clients[n].oSD == j){
					observer = 1;
				}
				if (clients[n].username[0] != '\0'){
					registered_username = 1;
					break;
				}
				else {
					break;
				}
			}
		}
		if(!observer){
			continue;
		}
		//only send to users with registered username
		if (FD_ISSET(j, master) && registered_username){
			if (j != lsdo && j != i && j != lsdp) {
				if(clients[n].pSD == j){
					continue;
				}
				if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
					perror("send");
				}
			}
		}
	}
}

void send_to_one(int i, int nbytes_recvd, char *recv_buf, fd_set *master){
	//parse username
	int username_len = 1;	//start at 1 since we already checked that the first char = @
	char uname[11];
	char new_message[BUFSIZE];
	char * sender;
	int message_len, user_found = 0;

	memset(uname, 0, 11);
	while (recv_buf[username_len] != ' '){
		uname[username_len - 1] = recv_buf[username_len];
		username_len++;
	}
	uname[username_len - 1] = '\n'; 


	//check if valid username
	for(int n = 0; n < MAXCLIENTS; n++){
		if (strcmp(clients[n].username, uname) == 0){
			//find sender username
			user_found = 1;
			for (int x = 0; x < MAXCLIENTS; x++){
				if (clients[x].pSD == i){
					sender = clients[x].username;
					break;
				}
			}
			int sender_len = strlen(sender);
			//get size of message in recv_buf
			for(int n = 0; n < BUFSIZE; n++){
				if (recv_buf[n] == '\n' || recv_buf[n] == '\0'){
					message_len = n;
					break;
				}
			}
			//subtract username and @symbol
			message_len = message_len - username_len - 1;
			
			//create private message format
			char delim[2] = {':', ' '};
			char spaces[12] = {'-', ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',};
			memset(new_message, 0, BUFSIZE);
			strncat(new_message, spaces, 12 - sender_len);
			strncat(new_message, sender, sender_len - 1);
			strncat(new_message, delim, 2);
			strncat(new_message, recv_buf + username_len + 1, message_len);
			int total_msg_len = strlen(new_message);

			//valid recipient, send to oSD
			//if (clients[n].pSD != lsdo && clients[n].pSD != lsdp && clients[n].pSD != i && FD_ISSET(clients[n].pSD, master)){
			if(clients[n].oSD){
				send(clients[n].oSD, new_message,total_msg_len,0);			//<---- update to oSD and send to sender observer
			}
				//send(i, new_message,total_msg_len,0 );
			//}	
			break;
		}
	}
	
	if (!user_found){
		//tell sender this was an invalid user
		char * warn = "Warning: ";
		char * exist = " doesn't exist...";
		char invalid_message[BUFSIZE];
		memset(invalid_message, 0, BUFSIZE);

		strncat(invalid_message, warn, strlen(warn));
		strncat(invalid_message, uname, username_len - 2);
		strncat(invalid_message, exist, strlen(exist));
		if (i != lsdo && i != lsdp && FD_ISSET(i, master)){
			send(i, invalid_message,strlen(invalid_message),0);     		//should be sent to senders oSD (not i)
		}
	}
	

}

//disconnect the client or participant
void disconnect_client(int sd, fd_set *master){
	char message[BUFSIZE] = "";
	for(int i = 0; i < MAXCLIENTS; i++){
		if (clients[i].pSD == sd){
			fflush(stdout);
			if (clients[i].oSD){
				close(clients[i].oSD);
				FD_CLR(clients[i].oSD, master);
			}
			//reset all information in client struct

			//boradcast disconnection message
			strcat(message, "User ");
			strcat(message, clients[i].username);
			strcat(message, " has left\n");
			send_to_clients(sd, strlen(message), message, master);

			memset(clients[i].username, 0, strlen(clients[i].username));
			clients[i].pSD = 0;
			clients[i].oSD = 0;
			clients[i].last_input_time = 0;
			close(sd);
			FD_CLR(sd, master);
			break;
		//reset observer info if observer
		}
		else if (clients[i].oSD == sd){
			clients[i].oSD = 0;
			close(sd);
			FD_CLR(sd, master);
			break;
		}
	}
}

//recieves input from participants and sends it to other participants	
void message_handler(int i, fd_set *master, int sockfd, int fdmax){
	int nbytes_recvd, j;
	char recv_buf[BUFSIZE], buf[BUFSIZE];
	memset(recv_buf, 0, BUFSIZE);
	fflush(stdout);
	//skip if the observer is sending data
	//for(int p = 0; p < MAXCLIENTS; p ++){
	//	if(clients[p].oSD == i){
	//		recv(i, recv_buf, BUFSIZE, 0);
	//		return;
	//	}
	//}
	
	if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) {
		if (nbytes_recvd == 0) {
			/* This is code for disconnecting a participant. We need to check 
			if the current client is a participant or observer */

			//find participant username and send 'username left' message
			memset(recv_buf, 0, BUFSIZE);
			char user_msg[5] = {'U', 's', 'e', 'r', ' '};
			char left_msg[9] = {' ', 'h', 'a', 's', ' ', 'l', 'e', 'f', 't'};
			strncat(recv_buf, user_msg, 5);
			
			//check if client is registered to a username 
			for (int n = 0; n < MAXCLIENTS; n++){
				if (clients[n].pSD == i && clients[n].username[0] != '\0'){
					strncat(recv_buf, clients[n].username, strlen(clients[n].username) - 1);
					strncat(recv_buf, left_msg, 9);
					nbytes_recvd = strlen(recv_buf);
					send_to_clients(i, nbytes_recvd, recv_buf, master);
					break;
				//else if observer, send different message
				}
				else if (clients[n].oSD == i){
					char *o_leave = "Observer has left";
					strncat(recv_buf, o_leave, strlen(o_leave));
					nbytes_recvd = strlen(recv_buf);
					send_to_clients(i, nbytes_recvd, recv_buf, master);
					break;
				}
			}	
		}
		else {
			printf("i (socket) is %d when no bytes recieved\n", i);
			fflush(stdout);
			perror("recv");
		}
		disconnect_client(i, master);
	}
	else { 
		int just_registered = 0;
		//search for client username existance
		for(int n = 0; n < MAXCLIENTS; n++){
			if (clients[n].pSD == i){
				if (clients[n].username[0] == '\0'){
					char message[BUFSIZE]= "";
					if(!valid_username(recv_buf, 0)){
						strcat(message, "N");
						send(i, message, strlen(message), 0);
						FD_CLR(i, master);
						return;
					}
					else{
						strcat(message, "Y");
                                                send(i, message, strlen(message), 0);
					}
					strncpy(clients[n].username, recv_buf, 11);
					just_registered = 1;

					//update last active time

					
					//send join message
					char user[11];
					char * user_msg = "User ";
					char *left_msg = " has joined";
					strncpy(user, recv_buf, 10);
					memset(recv_buf, 0, BUFSIZE);
					strncat(recv_buf, user_msg, 5);
					int name_len = strlen(user);
					strncat(recv_buf, user, name_len - 1);
					strncat(recv_buf, left_msg, 11);
					int len = strlen(recv_buf);
					send_to_clients(i, len, recv_buf, master);
					break;
				} 
				else {
					//update last active time

					//only send to users with a registered username
					if(recv_buf[0] == '@'){
						send_to_observers(i, nbytes_recvd, recv_buf, master);
					}
					else {
						send_to_observers(i, nbytes_recvd, recv_buf, master);
					}

					
					break;
				}
			}
		}	
		//sent message is from an observer containing username
		if (!just_registered){
			for (int n = 0; n < MAXCLIENTS; n++){
				if (strncmp(clients[n].username, recv_buf, 11) == 0){
					clients[n].oSD = i;
				}
			}
		}
	}
}

//accept participants and register their usernames	
void connection_accept_participant(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr){
	socklen_t addrlen;
	int newsockfd;
	
	addrlen = sizeof(struct sockaddr_in);
	if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) {
		perror("accept");
		exit(1);
	}
	else {
		FD_SET(newsockfd, master);
		if(newsockfd > *fdmax){
			*fdmax = newsockfd;
		}
		for(int i = 0; i < MAXCLIENTS; i++){
			if(!clients[i].pSD){
				clients[i].pSD = newsockfd;
				time_t rawtime;
  				struct tm * timeinfo;
  				time ( &rawtime );
				clients[i].last_input_time = rawtime;
				break;
			}
		}
	}
}

//accept observer connections and update fdmax	
void connection_accept_observer(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr){
	socklen_t addrlen;
	int newsockfd;

	addrlen = sizeof(struct sockaddr_in);
	if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) {
		perror("accept");
		exit(1);
	}
	else {
		FD_SET(newsockfd, master);
		if(newsockfd > *fdmax){
			*fdmax = newsockfd;
		}
	}

}

//handle incoming connections with the specified port
void connect_init(int *sockfd, struct sockaddr_in *my_addr, int port1, int port2){
	int yes = 1;
		
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
		
	my_addr->sin_family = AF_INET;
	my_addr->sin_port = htons(port1);
	my_addr->sin_addr.s_addr = INADDR_ANY;
	memset(my_addr->sin_zero, '\0', sizeof my_addr->sin_zero);
		
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
		
	if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
	}
	if (listen(*sockfd, 10) == -1) {
		perror("listen");
		exit(1);
	}
}

int main(int argc, char **argv){
	if (argc != 3){
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./server participant-port observer-port\n");
		exit(EXIT_FAILURE);
	}
	fd_set allfd;
	fd_set currfd;
	int fdmax;

    struct sockaddr_in server_addr1, client_addr, observer_addr, server_addr2;
	
	FD_ZERO(&allfd);
	FD_ZERO(&currfd);
	connect_init(&lsdp, &server_addr1, atoi(argv[1]), atoi(argv[2]));
	connect_init(&lsdo, &server_addr2, atoi(argv[2]), atoi(argv[1]));
	FD_SET(lsdp, &allfd);
	FD_SET(lsdo, &allfd);
	fdmax = lsdo;

	while(1){
		currfd = allfd;
		//check for participants
		if(select(fdmax+1, &currfd, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		
		for (int i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &currfd)){
				//accept incomming participant
				if (i == lsdp){
					connection_accept_participant(&allfd, &fdmax, lsdp, &client_addr);
				}
				//accept incomming observer
				else if (i == lsdo){
					connection_accept_observer(&allfd, &fdmax, lsdo, &observer_addr);
				}
				else{
					//check if too much time passed
					// if not send message like normal
					time_t last_time, curr_time;
					//if observer dont check for time
					int observer = 0;
					int j = 0;
					for(j = 0; j < MAXCLIENTS; j ++){
						if(clients[j].oSD == i){
							observer = 1;
							message_handler(i, &allfd, lsdp, fdmax);
							break;
						}
						if(clients[j].pSD == i){
							last_time = clients[j].last_input_time;
							break;
						}

					}
					if(observer){
						continue;
					}
					time(&curr_time);
					time_t difference = curr_time - last_time;

					//handler if client took too long
					if(difference > 1000){
						disconnect_client(i, &allfd);
					}
					//client responded in time
					else if(difference <= 1000){
						message_handler(i, &allfd, lsdp, fdmax);
						time(&curr_time);
						clients[j].last_input_time = curr_time;
					}
				}
			}
		}
	}
	return 0;
}
