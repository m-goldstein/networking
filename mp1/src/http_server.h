#ifndef HTTP_SERVER_H
#define HTTP_SREVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#define PORT "80"
#define PATH_LENGTH 64
#define BACKLOG 10
#define BUF_SIZE 512

void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);

int handle_request(int sockfd);


#endif
