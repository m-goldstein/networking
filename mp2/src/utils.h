#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

//#define DEBUG
//#define PROGRESS
//#define GDB

#define SEQ_IDX         0x0
#define SEQ_BYTES       0x4
#define FLAG_IDX        (SEQ_IDX+SEQ_BYTES)
#define PKT_SIZE_IDX    (FLAG_IDX+1)
#define WINDOW_LEN      1096
#define RCV_WINDOW_LEN  (WINDOW_LEN-1)
#define PKT_SIZE        256
#define PKT_HEADER_SIZE (PKT_SIZE_IDX+1)
#define PKT_DATA_SIZE   (PKT_SIZE-PKT_HEADER_SIZE)
#define BUF_SIZE        (WINDOW_LEN*PKT_DATA_SIZE)
#define TIMER_AVG_SIZE  (WINDOW_LEN)
#define convert_to_microsec(time) \
    ((suseconds_t)(time.tv_sec * 1000000) + time.tv_usec)
/*  PACKET ARRANGEMENT
 *   0               32
 *  +----------------+
 *  | SEQUENCE NUMBER|
 *  +----------------+
 *  | ACK NUMBER     |
 *  +----------------+
 *  |XXXXXASF--------|
 *  +----------------+
 *          8
 */
/* for short-hand convenience */
typedef struct sockaddr SA;
typedef struct sockaddr_in SA_in;
typedef struct addrinfo AI;
typedef struct sockaddr_storage SAS;
enum FLAGS {
    ACK = 0x4,
    //SYN_ACK = 0x10,
    SYN = 0x2,
    //FIN_ACK = 0x10000,
    FIN = 0x1
};

struct connection {
    int16_t socket;
    char name[INET6_ADDRSTRLEN];
    uint16_t port;
    struct addrinfo* p;
    struct addrinfo* servinfo;
    struct sockaddr_in src;
    struct sockaddr_in dst;
    struct sockaddr_in* other;
    //struct sockaddr_storage dest;
    socklen_t slen;
    void (*destroy)(struct connection);
    //int16_t (*recv_header)(struct connection*);
    //int16_t (*send_header)(struct connection*);
    int16_t (*send_data) (struct connection*, uint8_t *buf, int size);
    int16_t (*recv_data) (struct connection*, uint8_t *buf, int size);
};


typedef struct connection udp_t;

int16_t recv_udp_header(udp_t *udp);
int16_t send_udp_header(udp_t *udp);

int16_t send_udp_data(udp_t *udp, uint8_t *buf, int size);
int16_t recv_udp_data(udp_t *udp, uint8_t *buf, int size);

void destroy_connection(udp_t udp);

void diep (char *s);

void print_as_binary(uint16_t n);

void* get_in_addr(struct sockaddr *sa);


void init_udp_recv(udp_t *udp);
void init_udp_send(udp_t *udp);

void set_timer(int sockfd, struct timeval timeout);

//void init_udp_recv_session(char *hostname, int *sockfd, unsigned short int port, struct addrinfo *p, struct addrinfo *servinfo, struct sockaddr_in *si_me, struct sockaddr_in *si_other, char *s, header_t* header);

//void init_udp_send_session(char *hostname, int *sockfd, unsigned short int port, struct addrinfo *p, struct addrinfo* servinfo, struct sockaddr_in *si_other, struct sockaddr_in* si_dst,char *s, header_t* header);
#endif
