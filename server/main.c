#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h> //shared mem

#define SERV_IP "127.0.0.1"
#define SERV_PORT   4110
#define P2P_PORT   4111
#define BACKLOG 10
#define INIT_MSG   "\n========================================\nHello! I'm P2P File Sharing Server...\nPlease, LOG-IN!\n========================================\n"
#define USER1_ID   "user1"
#define USER1_PW   "passwd1"
#define USER2_ID   "user2"
#define USER2_PW   "passwd2"
#define USER3_ID   "user3"
#define USER3_PW   "passwd3"
#define MAX_LEN 512

int *chat_header;
char *chat_data;
char *chat_user;
char *mutex;

int check_user(char*, char*, char*, int);
void chat_func(char*, char*, char*, char*, int);

int main(void)
{
	/* listen on sock_fd, new connection on new_fd */
	int sockfd, new_fd;

	/* my address information, address where I run this program */
	struct sockaddr_in my_addr;

	/* remote address information */
	struct sockaddr_in their_addr;
	unsigned int sin_size;

	/* buffer */
	int rcv_byte;
	char buf[512];

	char id[20];
	char pw[20];

	char msg[512];

	int val = 1;

	pid_t pid;

	/* socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Server-socket() error lol!");
		exit(1);
	}
	else printf("Server-socket() sockfd is OK...\n");

	/* host byte order */
	my_addr.sin_family = AF_INET;

	/* short, network byte order */
	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* zero the rest of the struct */
	memset(&(my_addr.sin_zero), 0, 8);

	/* to prevent 'Address already in use...' */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)& val, sizeof(val)) < 0) {
		perror("setsockopt");
		close(sockfd);
		return -1;
	}

	/* bind */
	if (bind(sockfd, (struct sockaddr*) & my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Server-bind() error lol!");
		exit(1);
	}
	else printf("Server-bind() is OK...\n");

	/* listen */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen() error lol!");
		exit(1);
	}
	else printf("listen() is OK...\n");

	chat_header = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	chat_data = mmap(NULL, sizeof(char) * MAX_LEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	chat_user = mmap(NULL, sizeof(char) * 20, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	mutex = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	*chat_header = 0;
	*mutex = 0;
	while (1)
	{
		/* ...other codes to read the received data... */
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr*) & their_addr, &sin_size);

		if (new_fd == -1) perror("accept() error lol");
		else printf("accept() is OK...\n\n");

		if (fork() == 0)
		{
			close(sockfd);

			memset(buf, 0, 512);

			/* send INIT_MSG */
			send(new_fd, INIT_MSG, strlen(INIT_MSG) + 1, 0);

			/* receive ID */
			rcv_byte = recv(new_fd, buf, sizeof(buf), 0);
			strncpy(id, buf, rcv_byte);

			/* receive PW */
			rcv_byte = recv(new_fd, buf, sizeof(buf), 0);
			strncpy(pw, buf, rcv_byte);

			/* print received user information */
			printf("=========================\n");
			printf("User Information\n");
			printf("ID: %s, PW: %s\n", id, pw);
			printf("=========================\n");

			/* check id & pw */
			if (check_user(id, pw, msg, new_fd)){
				//login
				sleep(1);
				send(new_fd, "good", strlen("good") + 1, 0);
				chat_func(id, pw, msg, buf, new_fd);
			} else {
				send(new_fd, "bad", strlen("bad") + 1, 0);
				printf("user unknown try to Log-in but fail\n");
			}


			close(new_fd);
			exit(1);
		}
		else {   //parent
			close(new_fd);
		}
	}
	munmap(chat_header, sizeof(*chat_header));
	munmap(chat_data, sizeof(*chat_data) * MAX_LEN);
	munmap(chat_user, sizeof(*chat_user) * 20);
	munmap(mutex, sizeof(*mutex));

	close(sockfd);

	return 0;
}

void chat_func(char *id, char *pw, char *msg, char *buf, int new_fd){
	char *exit_flag;
	int temp, r_byte;
	exit_flag = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	*exit_flag = 0;
	if(fork() == 0){
		//child -> only read shared mem
		temp = *chat_header;
		while(*exit_flag == 0){
			if(temp != *chat_header){
				//read
				temp = *chat_header;
				if (strcmp(id, chat_user) != 0){
					if(strcmp("Log-out", chat_data) == 0){
						while(*mutex){}
						*mutex = 1;
						//printf("##### %s %s #####\n", chat_user, chat_data);
						sprintf(msg, "##### %s %s #####", chat_user, chat_data);
						send(new_fd, msg, strlen(msg) + 1, 0);
						*mutex = 0;
					} else {
						while(*mutex){}
						*mutex = 1;
						//printf("%s : %s\n", chat_user, chat_data);
						sprintf(msg, "%s : %s", chat_user, chat_data);
						send(new_fd, msg, strlen(msg) + 1, 0);
						*mutex = 0;
					}
				}
			}
		}
	} else {
		//parent -> only recieve client's data
		while(*exit_flag == 0){
			memset(buf, 0, 512);
			r_byte = recv(new_fd, buf, MAX_LEN, 0);
			if(r_byte != -1){
				if(strcmp(buf, "/exit") == 0){
					*exit_flag = 1;
					while(*mutex){}
					*mutex = 1;
					memset(chat_data, 0, MAX_LEN);
					strncpy(chat_data, "Log-out", strlen("Log-out") + 1);
					strncpy(chat_user, id, strlen(id) + 1);
					printf("##### %s %s #####\n", chat_user, chat_data);
					*chat_header += 1;
					*mutex = 0;
				} else {
					while(*mutex){}
					*mutex = 1;
					memset(chat_data, 0, MAX_LEN);
					strncpy(chat_data, buf, strlen(buf) + 1);
					strncpy(chat_user, id, strlen(id) + 1);
					printf("%s : %s\n", chat_user, chat_data);
					*chat_header += 1;
					*mutex = 0;
				}
			}
		}
	}
	munmap(exit_flag, sizeof(char));
}

int check_user(char *id, char *pw, char *msg, int new_fd){
	if (strcmp(id, USER1_ID) == 0) {
		if (strcmp(pw, USER1_PW) == 0) {
			strcpy(msg, "Log-in success!! [user1] *^^*\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
		}
		else {
			strcpy(msg, "Log-in fail: Incorrect password...\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
			return 0;
		}
	} else if (strcmp(id, USER2_ID) == 0) {
		if (strcmp(pw, USER2_PW) == 0) {
			strcpy(msg, "Log-in success!! [user2] *^^*\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
		}
		else {
			strcpy(msg, "Log-in fail: Incorrect password...\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
			return 0;
		}
	} else if (strcmp(id, USER3_ID) == 0) {
		if (strcmp(pw, USER3_PW) == 0) {
			strcpy(msg, "Log-in success!! [user3] *^^*\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
		}
		else {
			strcpy(msg, "Log-in fail: Incorrect password...\n\n");
			send(new_fd, msg, strlen(msg) + 1, 0);
			printf("%s", msg);
			return 0;
		}
	} else {
		strcpy(msg, "Log-in fail: There is no correponding ID...\n\n");
		send(new_fd, msg, strlen(msg) + 1, 0);
		printf("%s", msg);
		return  0;
	}
	return 1;
}


