#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "80"
#define MAXDATASIZE 512
#define BUF_SIZE 256
void *get_in_addr(struct sockaddr *sa);
void extract_file_path(char* str, char** host, char**port, char** filename);
void make_http_header(char** header, char* url, char* filename);
int init_http_session(int *socket, struct addrinfo * addr, struct addrinfo* servinfo, struct addrinfo* p, char *s, char* host, char* port);
#endif
