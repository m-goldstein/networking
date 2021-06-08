#ifndef HTTP_CLIENT_C
#define HTTP_CLIENT_C
#include "http_client.h"

void extract_file_path(char* str, char** host, char** port, char** filename)
{
    if (*host == NULL) {
        *host = (char*) malloc(sizeof(char) * 64);
    }

    if (*port == NULL) {
        *port = (char*) malloc(sizeof(char) * 16);
    }

    if (*filename == NULL) {
        *filename = (char*) malloc(sizeof(char) * 64);
    }
    char* tmp_host = (char*)malloc(sizeof(char)*64);
    int port_start = 0;
    int port_end = 0;
    int start_idx = 0;
    int slash_idx = 0;
    char def_port[] = "80";
    char preamble[] = "http://";
    if (strncmp(str, preamble, sizeof(preamble)-1) == 0) {
        start_idx = sizeof(preamble)-1;
    } else {
        start_idx = 0;
    }
    sscanf(str+start_idx, "%s%*[^:]%*c", tmp_host);
    int j ;
    char c = '/';
    for (j = start_idx; j < strlen(tmp_host); j++) {
        if ((tmp_host)[j] == ':') {
            port_start = j;
            continue;
        }
        else if ( ( (tmp_host)[j] == '/') && port_start != 0) {
            port_end = j;
            break;
        }
        if ((tmp_host)[j] == '/') {
            slash_idx = j;
        }
    }

    if (slash_idx == 0) {
        strcat(tmp_host, &c);
        slash_idx = strlen(tmp_host)-1;
        //strcpy(*filename, (tmp_host+slash_idx));
    } else {
        //strncpy(*filename, (tmp_host), strlen(str)-1);
    }
    if (port_end <= port_start) {
        strncpy(*port, def_port, 2);
    }
    else {
        strncpy(*port, (tmp_host)+port_start+1, port_end - port_start-1);
    }
    if (port_start != 0) {
        strncpy(*host, tmp_host, port_start);
    } else {
        strncpy(*host, tmp_host, slash_idx);
    }
    strcpy(*filename, str + start_idx +port_end );
    //strncpy(*host, tmp_host, slash_idx);
    printf("[DEBUG]: input string: %s\n", str);
    printf("[DEBUG]: host string: %s\n", *host);
    printf("[DEBUG]: port stirng: %s\n", *port);
    printf("[DEBUG]: filepath: %s\n", *filename);
}

void make_http_header(char** header, char* url, char* filename)
{
    char formatter[] = "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n";
    *header = (char*)malloc((strlen(formatter) + strlen(filename) + strlen(url)) * sizeof(char));
    
    sprintf(*header, formatter, filename, url); 
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int init_http_session(int *sock, struct addrinfo *addr, struct addrinfo* servinfo, struct addrinfo* p, char *s, char* host, char*port)
{
    int rv;
    if (sock == NULL) {
        sock = (int*)malloc(sizeof(int));
    } 
    if (addr == NULL) {
        addr = (struct addrinfo*) malloc(sizeof(struct addrinfo));
    }
    if (p == NULL) {
        p = (struct addrinfo*) malloc(sizeof(struct addrinfo));
    }
    if (s == NULL) {
        s = (char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
    }
    memset(addr, 0, sizeof(*addr));
    addr->ai_family = AF_INET;
    addr->ai_socktype = SOCK_STREAM;
    //int _port = atoi(port);
    //printf("%d", _port);
    printf("PORT: %s\n", port);
    if ((rv = getaddrinfo(host, port, addr, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((*sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket\n");
            continue;
        }
        if (connect(*sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sock);
            perror("client: connect.\n");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect.\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    return 0;
}

int main(int argc, char* argv[])
{
    int sockfd; 
    //char s[INET6_ADDRSTRLEN];
    char *s = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    servinfo = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    p = (struct addrinfo*)malloc(sizeof(struct addrinfo)); 
    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }
    char* filepath = NULL;
    char* host = NULL; 
    char* port = NULL;
    extract_file_path(argv[1], &host, &port, &filepath); 
    printf("HOST: %s\nPORT: %s\nFILE: %s\n", host, port, filepath);
    init_http_session(&sockfd, &hints, servinfo, p, s, host, port); 
    //inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    printf("client: connecting to %s\n", (s));
    printf("Connected!\n");
    int nbytes = 0;
    char* header;
    make_http_header(&header, host, filepath);
    printf("HEADER:\n %s\n", header);
    while (1) {
        nbytes = send(sockfd, header, strlen(header)+1, 0);
        printf("sent: %d bytes.\n", nbytes);
        break;
    }
    memset(buf, 0, MAXDATASIZE);
    char c[BUF_SIZE];
    //int i = 0;
    FILE *fp;
    fp = fopen("output", "w");
    int peel_header = 0;
    while ( (nbytes = recv(sockfd, c, BUF_SIZE, 0)) > 0) {
        if(!peel_header) {
            for (int i = 0; i < nbytes; i++) {
                if (c[i] == '\r' && c[i+1] == '\n' &&
                    c[i+2] == '\r' && c[i+3] == '\n') {
                    fwrite((c+i+4), sizeof(char), nbytes-(i+4), fp);
                    peel_header = 1;
                }
            }
        }
        else {
            if (nbytes < BUF_SIZE) {
                fwrite(c, sizeof(char), nbytes, fp);
                //printf("BYTES READ: %d\n", nbytes);
                //break;
            }
            else {
                fwrite(c, sizeof(char), BUF_SIZE, fp);
                //printf("BYTES READ: %d\n", nbytes);
            }
        }

    }
    //buf[i+1] = '\0';
    //printf("client: received '%s'\n", buf);

    freeaddrinfo(servinfo);
    close(sockfd);
    fclose(fp);
    free(host);
    free(port);
    free(filepath);
    free(header);
    //free(sockfd);
    return 0;
}
#endif
