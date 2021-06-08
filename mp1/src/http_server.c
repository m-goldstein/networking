/*
 * http_server.c - a http server daemon
 */

#ifndef HTTP_SERVER_C
#define HTTP_SERVER_C

#include "http_server.h"
/* signal handler, as shown in the demo files */
void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* get sockaddr, IPv4 or IPv6, as shown in the demo server.c file */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
    int sockfd, new_fd;
    struct addrinfo sa, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    struct sigaction sigact;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char* port;
    if (argc == 2) {
        port = (char*)malloc(sizeof(char) * strlen(argv[1]));
        bzero(port, strlen(argv[1]));
        strncpy(port, argv[1], strlen(argv[1]));
    } else if (argc != 2) {
        port = (char*)malloc(sizeof(char)*5);
        bzero(port, 5);
        sprintf(port, "8888");
        printf("No port specified. Using default %s.\n", port);
    }
    memset(&sa, 0, sizeof(sa));
    sa.ai_family = AF_INET;
    sa.ai_socktype = SOCK_STREAM;
    sa.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &sa, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket\n");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt\n");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind.\n");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen\n");
        return 1;
    }
    sigact.sa_handler = sigchld_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sigact, NULL) == -1) {
        perror("sigaction\n");
        return 1;
    }
    printf("server initialized: waiting for connections on port %s.\n", port);
    while (1) {
        sin_size = sizeof(client_addr);
        new_fd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept\n");
            continue;
        }
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr*)&client_addr), s, sizeof(s));
        printf("server: connected to %s\n", s);
        if ((!fork())) {
            close(sockfd);
            handle_request(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    
    return 0;
}

int handle_request(int sockfd) 
{
    // Make sure it's a valid fd
    if (sockfd <= 0) { 
        return -1; 
    }
    FILE* fp;
    struct stat file_stats;
    char* buf;
    int bytes_read;
    int is_valid_request = 0;
    int is_valid_file = 0;
    int response_len = 0;
    char header[3][BUF_SIZE];
    char* f_buf;
    int rv;
    char* filename;
    char* absolute_path;
    char pwd[PATH_LENGTH];
    char c = '/';
    int i;
    int file_start = -1;
    int file_end = -1;
    size_t f_size;
    // Initialize vars for response
    bzero(header[0], BUF_SIZE); 
    bzero(header[1], BUF_SIZE);
    bzero(header[2], BUF_SIZE);
    // Read the request header to make sure it's a GET request
    buf = (char*)(malloc(sizeof(char) * BUF_SIZE));
    bzero(buf, BUF_SIZE);
    if ( (bytes_read = recv(sockfd, buf, BUF_SIZE - 1, 0)) < 0) {
        is_valid_request = 0;
    }
    // If it's not a GET, or just garbage, throw it away
    if (sscanf(buf, "%s %s %s", header[0], header[1], header[2]) == -1) {
        //printf("%x\n", buf);
        printf("Bad header?\n");
        is_valid_request = 0;
    }
    //printf("GOT: [%s] [%s] [%s]\n", header[0], header[1], header[2]); 
    /* send a bad request if not a GET */
    if (strcmp(header[0], "GET") != 0) {
        is_valid_request = 0;
    } else { /* otherwise proceed */
        is_valid_request = 1;
    }
    if (!is_valid_request) {
        char bad_request[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
        //fwrite(bad_request, 1, sizeof(bad_request), resp);
        response_len += sizeof(bad_request);
        send(sockfd, bad_request, sizeof(bad_request)-1, 0);
    } else {
        /* parse filename */
        for (i = 0; i < strlen(header[1]); i++) {
            if (header[1][i] == c) {
                file_start = i;
                file_end = strlen(header[1]) - file_start;
                break;
            }
        }
        /* dynamically allocate space for the bookkeeping */
        if (file_end - file_start > 0) {
            filename = (char*)malloc(sizeof(char)*(file_end-file_start));
        } else {
           filename = (char*)malloc(sizeof(char));
            *filename = c;
            file_end = 2;
            file_start = 1;
        }
        /* file_start + 1 added to remove leading '/'*/
        strncpy(filename, header[1]+file_start+1, file_end-file_start);
        /* take the current working directory to make sure the rest of the 
           filesystem cant be accessed ie /etc/passwd */
        getcwd(pwd, sizeof(pwd));
        //printf("Read filename:%s %s\n", pwd, filename);
        absolute_path = (char*) malloc(sizeof(char) * (sizeof(pwd) + sizeof(filename)+1));
        bzero(absolute_path, sizeof(pwd) + sizeof(filename)+1);
        strncpy(absolute_path, pwd, sizeof(pwd)-1);
        strncat(absolute_path, &c, 1);
        strncat(absolute_path, filename, file_end - file_start);
        // Make sure the file is valid, otherwise send a 404
        if ((fp = fopen(absolute_path, "r")) == NULL) {
            printf("Requested file: %s not found. :(\n", absolute_path);
            is_valid_file = 0;
        }   else {
                /* use stat sys call to pull up file information */
                if((rv = stat(absolute_path, &file_stats)) < 0) {
                    is_valid_file = 0;
                }
            /* check the priviledges to prevent arbitrary file access */
            if ((file_stats.st_mode & S_IRUSR) ) {
                f_size = file_stats.st_size;
                is_valid_file = 1;
                printf("Surely, streaming %s to a random user over the web is reasonable.\n", absolute_path);
            } else {
                is_valid_file = 0;
                printf("Probably a bad idea to send %s to a random connection...\n", absolute_path);
            }
        }
        /* copy file contents into new buffer */
        if (is_valid_file) {
            f_buf = (char*)malloc(sizeof(char)*f_size);
            /* generate an HTTP OK header */
            char http_ok[] = "HTTP/1.1 200 OK\r\n\r\n"; //Content-Length: %d\r\n";
            send(sockfd, http_ok, sizeof(http_ok)-1, 0);
            bzero(f_buf, f_size);
            fread(f_buf, 1, f_size, fp);
            fclose(fp);
            printf("Server says OK!\n");
            send(sockfd, f_buf, f_size, 0);
            free(f_buf);
        } else if (!is_valid_file && is_valid_request){
        /* generate a 404 error */
            char http_fof[] = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(sockfd, http_fof, sizeof(http_fof)-1, 0);
            printf("Server says NO WAY!\n");
        }
    }
    //char http_eof[] = "\r\n";
    //send(sockfd, http_eof, sizeof(http_eof), 0);
    free(buf);
    close(sockfd);
    free(filename);
    free(absolute_path);
    return 0;
}

#endif
/*
            // Get length of file
            fseek(fp, 0, SEEK_END);
            int f_len = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // Copy full file to new buffer
            char* f_buf = (char*)malloc(sizeof(char)*f_len);
            fread(f_buf, sizeof(char), f_len, fp);
            fclose(fp);

            // Create the response
            response = (char*)malloc(sizeof(char)*(f_len + 32));
            response_len = f_len + 32;
            sprintf(response, "HTTP/1.1 200 OK\n\r\n");
            //write(response
            strncat(response, f_buf, f_len);
            //char c = "\n";
            //strcat(response, &c); 
            free(f_buf);
        }
        free(file);
        free(fp);
        break;
    }
}*
        int file_start = -1;
        int file_end = -1;
        while((read = recv(sockfd, buf, BUF_SIZE, 0)) > 0) {
            if (read < 0) {
                printf("ERROR: handle_request recv(): %i", read);
                return -1;
            }
            for (int i = 0; i < read; i++) {
                if (buf[i] == '/') {
                    file_start = i + 1;
                    for (int j = i + 1; j < read; j++) {
                        if (buf[j] == ' ') {
                            file_end = j;
                            break;
                        }
                        // It's technically possible to get a filename 
                        // longer than BUF_SIZE but it's sooooooo unlikely
                    }
                    break;
                }
            }
            // We can probably clean this bit up as there's some unnecessary
            // logic assuming we don't allow arbitrarily sized file lengths
            if (file_end != -1) {
                file = (char*)(malloc(sizeof(char) * (file_end - file_start)));
                strncpy(file, (buf + file_start), file_end - file_start);
                FILE *fp;
                // Make sure the file is valid, otherwise send a 404
                if ((fp = fopen(file, "r")) == NULL) {
                    printf("Requested file: %s\n", file);
                    response = (char*)malloc(sizeof(char) * 26);
                    response_len = 26;
                    sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                }
                else {
                    // Get length of file
                    fseek(fp, 0, SEEK_END);
                    int f_len = ftell(fp);
                    fseek(fp, 0, SEEK_SET);

                    // Copy full file to new buffer
                    char* f_buf = (char*)malloc(sizeof(char)*f_len);
                    fread(f_buf, sizeof(char), f_len, fp);
                    fclose(fp);

                    // Create the response
                    response = (char*)malloc(sizeof(char)*(f_len + 32));
                    response_len = f_len + 32;
                    sprintf(response, "HTTP/1.1 200 OK\n\r\n");
                    //write(response
                    strncat(response, f_buf, f_len);
                    //char c = "\n";
                    //strcat(response, &c); 
                    free(f_buf);
                }
                free(file);
                free(fp);
                break;
            }
        }*/

