#ifndef _UTILS_C
#define _UTILS_C
#include "utils.h"
void destroy_connection(udp_t udp)
{
    printf("destroying udp connection: %s:%d\n", udp.name, udp.port);
    if (udp.p != NULL) {
        free(udp.p);
        udp.p = NULL;
    } if (udp.servinfo != NULL) {
        freeaddrinfo(udp.servinfo);
        //free(udp.servinfo);
        udp.servinfo = NULL;
    }
    memset(udp.name, 0, INET6_ADDRSTRLEN);
    //memset(&udp.header, 0, sizeof(header_t)); 
    memset(&udp.src, 0, sizeof(SA_in));
    memset(&udp.dst, 0, sizeof(SA_in));
    memset(udp.other, 0, sizeof(SA_in));
}

void set_timer(int sockfd, struct timeval timeout)
{
    int rv;
    struct timeval to;

    timerclear(&to); // ensure its not storing garbage
    //to.tv_sec = timeout.tv_sec - (timeout.tv_sec/60); // add 5% margin of error
    //to.tv_usec = timeout.tv_usec + (timeout.tv_usec/60); // add 5% margin of error

    to.tv_sec = (timeout.tv_sec < 0) ? 0 : timeout.tv_sec;
    to.tv_usec = (timeout.tv_usec < 0) ? 2500:timeout.tv_usec;
    if ((rv = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&to, (socklen_t)sizeof(to))) < 0) {
        //fprintf(stderr, "error setting recv setsockopt\n");
        //fflush(stderr);
    }
    if ((rv = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void*)&to, (socklen_t)sizeof(to))) < 0) {
        //fprintf(stderr, "error setting send setsockopt\n");
        //fflush(stderr);
    }
}
/*int16_t recv_udp_header(udp_t* udp)
{
    int nbytes;
    char cur_pkt[PKT_SIZE];
    uint16_t flags = -1;
    udp->slen = (socklen_t)sizeof(udp->dst);
    if ((nbytes = recvfrom(udp->socket, (void*)cur_pkt, PKT_SIZE, 0x0, (SA*)&udp->dst, (&udp->slen))) != 0) {
        flags = *((uint16_t*)&cur_pkt) & 0x1F;
        //udp.header = (*(header_t*)&flags);
    }
    return flags;
}
int16_t send_udp_header(udp_t* udp)
{
    int nbytes;
    char cur_pkt[PKT_SIZE];
    udp->slen = (socklen_t)sizeof(udp->dst);
    if (sendto(udp->socket, (void*)&udp->header, sizeof(header_t), 0, (SA*)&udp->dst, (socklen_t)sizeof(udp->dst)) == -1) {
        fprintf(stderr, "error sending udp header on socket %d\n", udp->socket);
        return -1;
    }
    return 0;
}
*/
int16_t send_udp_data(udp_t *udp, uint8_t *buf, int size)
{
    if (udp == NULL || buf == NULL)
        return -1;
    int nbytes = 0;
    //int acc = 0;
    //int errco = 0;
    udp->slen= (socklen_t)sizeof(udp->dst);
    //udp->slen = (socklen_t)sizeof(*udp->other);
    if ((nbytes = sendto(udp->socket, (void*)buf, size, 0, (SA*)&udp->dst, udp->slen)) == -1) {
        return -1;;
    }
    /*
    do {
        acc = 0;
        if ((acc = sendto(udp->socket, buf, 1, 0, (SA*)&udp->dst, udp->slen)) < 0) {
            errco += 1;
        }
        nbytes += acc;
    } while (errco < 9 && nbytes < size);
    */
    return nbytes;
}
int16_t recv_udp_data(udp_t *udp, uint8_t *buf, int size)
{
    if (udp == NULL || buf == NULL) {
        return -1;
    }
    int nbytes = 0;
    //int acc = 0;
    udp->slen = (socklen_t)sizeof(udp->dst);
    //memset(&udp->header, 0, sizeof(header_t));
    
    if ((nbytes = recvfrom(udp->socket, (void*)buf, size, 0x0, (SA*)&udp->dst, &udp->slen)) == -1) {
        return -1;
    }
    return nbytes;
    /*
    while ( nbytes < size) {
        acc = recvfrom(udp->socket, buf, size, 0x0, (SA*)&udp->dst, &udp->slen);
        if (acc < 0)
            return -1;
        nbytes += acc;
        printf("recieved: %d bytes\n", acc);
        udp->recv_header(&udp);
        if (udp->header.FIN == 1) {
            memset(&udp->header, 0, sizeof(header_t));
            udp->header.FIN_ACK = 1;
            udp->send_header(&udp);
            break;
        }
    }*/

    //return nbytes;
}

void print_as_binary(uint16_t n)
{
    while (n) {
        if (n & 1) {
            printf("1");
        } else {
            printf("0");
        }
        n >>= 1;
    }
    printf("\n");
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (((struct sockaddr*)&sa)->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void diep(char *s) 
{
    perror(s);
    exit(1);
}

void init_udp_send(udp_t* udp)
{
    if (udp == NULL)
        return;

    if (udp->p == NULL) {
        udp->p = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    }    
    if (udp->servinfo == NULL) {
        udp->servinfo = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    } if (udp->other == NULL) {
        udp->other = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    }
    udp->destroy = &destroy_connection;
    //udp->recv_header = &recv_udp_header;
    //udp->send_header = &send_udp_header;
    udp->send_data = &send_udp_data;
    udp->recv_data = &recv_udp_data;
    struct addrinfo hints;
	int rv;
	//int numbytes;
    //int slen = sizeof(*udp->other);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
    char* _port = (char*) malloc(sizeof(char)*5);
    sprintf(_port, "%d", udp->port);
	if ((rv = getaddrinfo(udp->name, _port, &hints, &udp->servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        free(_port);
		return;
	}
    free(_port);

	// loop through all the results and make a socket
	for(udp->p = udp->servinfo; udp->p != NULL; udp->p = udp->p->ai_next) {
		if ((udp->socket = socket(udp->p->ai_family, udp->p->ai_socktype,
				udp->p->ai_protocol)) < 0) {
			perror("sender: socket");
			continue;
		}

		break;
	}

	if (udp->p == NULL) {
		fprintf(stderr, "sender: failed to bind socket\n");
		return;
	}
    memset(&udp->dst, 0, sizeof(struct sockaddr_in));
    udp->dst.sin_family = AF_INET;
    udp->dst.sin_port = htons(udp->port);//htons((uint16_t)atoi(port));
    inet_pton(AF_INET, udp->name, &udp->dst.sin_addr);

    memset((void*)udp->other, 0, sizeof(struct sockaddr_in));
    udp->other->sin_family = AF_INET;
    udp->other->sin_port = htons(((struct sockaddr_in*)udp->p->ai_addr)->sin_port);
    udp->other->sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(udp->socket, (struct sockaddr*)udp->other, sizeof(struct sockaddr_in)) == -1) {
        close(udp->socket);
        perror("init udp send error.\n");
        exit(2);
    }

    struct timeval to;
    to.tv_sec = 5;
    to.tv_usec = 500000;
    set_timer(udp->socket, to); // default timeout
}

void init_udp_recv(udp_t* udp)
{
    //struct addrinfo hints;
	//int rv;
	//int numbytes;
	//struct sockaddr_storage their_addr;
	//socklen_t addr_len;
    if (udp == NULL) {
        udp = (udp_t*) malloc(sizeof(udp_t));
    }
    char *_port  = (char*) malloc(sizeof(unsigned short int)*5);
    sprintf(_port, "%d", udp->port);
    
    udp->destroy = &destroy_connection;
    //udp->recv_header = &recv_udp_header;
    //udp->send_header = &send_udp_header;
    udp->send_data = &send_udp_data;
    udp->recv_data = &recv_udp_data;
    //udp.destroy(udp);
    if (udp->socket != 0) {
        udp->socket = 0;
    } if (udp->p == NULL) {
        udp->p = (AI*)malloc(sizeof(SA));
    } if (udp->servinfo == NULL) {
        udp->servinfo = (AI*)malloc(sizeof(SA));
    }
	if ((udp->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    memset((char*)&udp->src, 0, sizeof(udp->src));
    udp->src.sin_family = AF_INET;
    udp->src.sin_port = htons(udp->port);
    udp->src.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding...\n");
    //udp->slen = (socklen_t)sizeof(udp->dst); 
    if (bind(udp->socket, (SA*)&udp->src, sizeof(udp->src)) == -1)
        diep("bind");
        
    struct timeval to;
    to.tv_sec = 5;
    to.tv_usec = 0;
    //to.tv_sec = 3;
    //to.tv_usec = 500000;
    set_timer(udp->socket, to); // default timeout
    
    free(_port);
	printf("started listen on port %d : waiting to recvfrom...\n", udp->port);
}
/* used beej's talker as a base for this */
/*void init_udp_recv_session(char *hostname, int *sockfd, unsigned short int port, 
        struct addrinfo *p, struct addrinfo *servinfo, struct sockaddr_in* si_me, struct sockaddr_in *si_other, 
        char *s, header_t *header)
{
	if (sockfd == NULL) {
        sockfd = (int*) malloc(sizeof(int));
    } if (p == NULL) {
        p = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    } if (s == NULL) {
        s = (int*) malloc(sizeof(int) * INET6_ADDRSTRLEN);
    } if (header == NULL) {
        header =(header_t*) malloc(sizeof(header));
    } if (si_me == NULL) {
        si_me = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    } if (si_other == NULL) {
        si_other = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    } if (servinfo == NULL) {
        servinfo = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    }
    struct addrinfo hints;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
    char *_port  = (char*) malloc(sizeof(unsigned short int)*5);
    sprintf(_port, "%d", port);
	if ((*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    memset((char*)si_me, 0, sizeof(*si_me));
    si_me->sin_family = AF_INET;
    si_me->sin_port = htons(port);
    si_me->sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding...\n");
    
    if (bind(*sockfd, (struct sockaddr*)si_me, sizeof(*si_me)) == -1)
        diep("bind");
        
    free(_port);
	printf("started listen on port %d : waiting to recvfrom...\n", port);
}

void init_udp_send_session(char* hostname, int* sockfd, unsigned short int port, struct addrinfo* p, 
        struct addrinfo* servinfo, struct sockaddr_in* si_other, struct sockaddr_in* si_dst, char *s, header_t* header)
{
    if (hostname == NULL) {
        hostname = (char*) malloc(sizeof(char) * 32);
    } if (sockfd == NULL) {
        sockfd = (int*) malloc(sizeof(int));
    } if (p == NULL) {
        p = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    } if (s == NULL) {
        s = (char*) malloc(sizeof(char) * INET6_ADDRSTRLEN);
    } if (servinfo == NULL) {
        servinfo = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    } if (header == NULL) {
        header = (header_t*)malloc(sizeof(header));
    } if (si_other == NULL) {
        si_other = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    } if (si_dst == NULL) {
        si_dst = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    }
    struct addrinfo hints;
	int rv;
	int numbytes;
    int slen = sizeof(*si_other);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
    char* _port = (char*) malloc(sizeof(char)*5);
    sprintf(_port, "%d", port);
	if ((rv = getaddrinfo(hostname, _port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        free(_port);
		return;
	}
    free(_port);

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((*sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "sender: failed to bind socket\n");
		return;
	}
    memset(si_other, 0, sizeof(struct sockaddr_in));
    si_other->sin_family = AF_INET;
    si_other->sin_port = htons(port);//htons((uint16_t)atoi(port));
    inet_pton(AF_INET, hostname, &si_other->sin_addr);

    memset(si_dst, 0, sizeof(struct sockaddr_in));
    si_dst->sin_family = AF_INET;
    si_dst->sin_port = htons(((struct sockaddr_in*)p->ai_addr)->sin_port);
    si_dst->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(*sockfd, (struct sockaddr*)si_dst, sizeof(struct sockaddr_in)) == -1) {
        close(*sockfd);
        perror("init udp send error.\n");
        exit(2);
    }

} */
#endif
