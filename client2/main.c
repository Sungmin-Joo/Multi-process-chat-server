#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h> //shared mem

#define SERV_IP      "127.0.0.1"
#define SERV_PORT   4110
#define P2P_PORT   4111

void chat_func(int, char*);

char *exit_flag;
int main(int argc, char *argv[ ])
{
	int sockfd; /* will hold the destination addr */
	struct sockaddr_in dest_addr;

	int rcv_byte;
	char buf[512];

	char id[20];
	char pw[20];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd == -1)
	{
		perror("Client-socket() error lol!");
		exit(1);
	}
	else printf("Client-socket() sockfd is OK...\n");

	/* host byte order */
	dest_addr.sin_family = AF_INET;

	/* short, network byte order */
	dest_addr.sin_port = htons(SERV_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(SERV_IP);

	/* zero the rest of the struct */
	memset(&(dest_addr.sin_zero), 0 ,8);

	/* connect */
	if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1){
		perror("Client-connect() error lol");
		exit(1);
	}
	else printf("Client-connect() is OK...\n");

	/* receive INIT_MSG */
	rcv_byte = recv(sockfd, buf, sizeof(buf), 0);
	printf("%s\n", buf);

	/* input & send ID */
	printf("ID : ");
	scanf("%s", id);
	send(sockfd, id, strlen(id) + 1, 0);

	/* input & send PW */
	printf("PW : ");
	scanf("%s", pw);
	send(sockfd, pw, strlen(pw) + 1, 0);

   	/* receive login result */
	rcv_byte = recv(sockfd, buf, sizeof(buf), 0);
	printf("%s",buf);

	memset(buf, 0, sizeof(buf));
	printf("log-in...\n");
	rcv_byte = recv(sockfd, buf, sizeof(buf), 0);
	if(strcmp(buf, "good") == 0){
		printf("log-in success\n");
		exit_flag = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		chat_func(sockfd, buf);
	}
	close(sockfd);
	return 0;
}

void chat_func(int sock, char *buf){
	int rcv_byte;
	exit_flag = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*exit_flag = 0;
	if(fork() == 0){
		//child -> only get message
		while (*exit_flag == 0){
			memset(buf, 0, 512);
			rcv_byte = recv(sock, buf, 512, 0);
			if(rcv_byte > 0){
				printf("%s\n", buf);
			}
		}
	} else {
		//parent -> only send message
		while (*exit_flag == 0){
			memset(buf, 0, sizeof(buf));
			scanf(" %[^\n]s", buf);
			if(strcmp(buf, "/exit") == 0){
				*exit_flag = 1;
			}
			send(sock, buf, strlen(buf) + 1, 0);
		}
	}
	munmap(exit_flag, sizeof(char));
}
