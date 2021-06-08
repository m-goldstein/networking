#ifndef _RECV_MAIN_C
#define _RECV_MAIN_C

/* 
 * File:   receiver_main.c
 * Author: Fawaz Tirmizi (fst2)
 *         Maximillian Goldstein (mgg2)
 *
 * Created on
 */


#include "utils.h"

struct sockaddr_in si_me, si_other;
int s, slen;

/*void diep(char *s) {
    perror(s);
    exit(1);
}*/



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) 
{
    FILE* fp;
    //char buf[BUF_SIZE];        // Packet data buffer
    uint8_t* buf = NULL;
    //char buf_recv[RCV_WINDOW_LEN] = {0};
    uint8_t* buf_recv = (uint8_t*)malloc(RCV_WINDOW_LEN);
    //buf_recv = (char*)realloc(buf_recv, RCV_WINDOW_LEN);
    //char *buf_recv = (char*)calloc(RCV_WINDOW_LEN, sizeof(char));
    buf = (uint8_t*)malloc(BUF_SIZE);
    memset((void*)buf, 0, BUF_SIZE);
    //char buf_recv[RCV_WINDOW_LEN];// Array noting recieved packets
    
    //memset((void*)buf_recv, 0, RCV_WINDOW_LEN);
    //uint8_t cur_pkt[PKT_SIZE] = {0};    // Current packet
    //uint8_t ACK_pkt[PKT_SIZE] = {0};
    //uint8_t pkt_sizes[PKT_SIZE] = {0};
    uint8_t *cur_pkt = (uint8_t*)malloc(PKT_SIZE);
    uint8_t *ACK_pkt = (uint8_t*)malloc(PKT_SIZE);
    uint8_t *pkt_sizes = (uint8_t*)malloc(PKT_SIZE);
    //memset((void*)pkt_sizes, 0, PKT_SIZE);
    //uint32_t cur_pkt_num = 0;            // ID of current packet
    int base = 0;               // Base idx of window
    int write_count = 0;        // Packets that can be written to file
    int nbytes;                 // Number of bytes received
    udp_t udp;
    udp.port = myUDPport;
    udp.p = NULL;
    udp.servinfo = NULL;
    udp.socket = -1;

    init_udp_recv(&udp);
    udp.destroy = &destroy_connection;
	/* Now receive data and send acknowledgements */    
    fp = fopen(destinationFile, "w");
    if (fp == NULL) {
       fprintf(stderr,"error could not open %s\n", destinationFile); 
       return;
    }
    // do 3-way handshake
    // Recieve SYN
    //memset(cur_pkt, 0, PKT_SIZE);
    //memset(ACK_pkt, 0, PKT_SIZE);
    printf("Waiting for SYN\n");
    do {
        while((nbytes = udp.recv_data(&udp, cur_pkt, PKT_SIZE)) == 0);
    } while((cur_pkt[FLAG_IDX] & SYN) == 0);
    printf("SYN recieved. Sending ACK. \n");
    ACK_pkt[FLAG_IDX] |= SYN | ACK;
    do {
        udp.send_data(&udp, ACK_pkt, PKT_SIZE);
        printf("ACK sent. Wating on ACK.\n");
        while( (nbytes = udp.recv_data(&udp, cur_pkt, PKT_SIZE)) == 0);
        printf("   Packet recieved...\n");
    } while((cur_pkt[FLAG_IDX] & ACK) == 0);
    
    printf("ACK recieved! Handshake complete!\n");
    struct timeval to;
    timerclear(&to);
    to.tv_sec = 0;
    to.tv_usec = 0;
    set_timer(udp.socket, to); // clear the timer, force it to wait
    //buf_recv = (char*)malloc(sizeof(char)*(1+RCV_WINDOW_LEN));
    //memset((void*)buf_recv, 0, sizeof(char)*(RCV_WINDOW_LEN+1));
    printf("Writing data to file\n");
    int done = 0;
    int count = 0;
    do {
        bzero(cur_pkt, PKT_SIZE);
        bzero(ACK_pkt, PKT_SIZE);
try_again:
        nbytes = udp.recv_data(&udp, cur_pkt, PKT_SIZE);
        if ((cur_pkt[FLAG_IDX] & FIN) == FIN) {
            printf("FIN recieved, ending\n");
            done = 1;
            break;
        }
        uint32_t cur_pkt_num = 0x0;
        cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX]) & 0xff; 
        cur_pkt_num <<= 8;
        cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+1]) & 0xff;
        cur_pkt_num <<= 8;
        cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+2]) & 0xff;
        cur_pkt_num <<= 8;
        cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+3]) & 0xff;
        // Move the packet data into the buffer, and mark the packet as recieved
        #ifdef DEBUG
        printf("Recieved packet: %X\n", cur_pkt_num);
        #endif
        //printf("");
        if (cur_pkt_num < base) {
            #ifdef DEBUG
            printf("Packet(=%X)  < base(=%X), resending ACK\n",cur_pkt_num, base);
            #endif
            udp.send_data(&udp, cur_pkt, PKT_SIZE_IDX);
            continue;
        }// || cur_pkt_num > base+WINDOW_LEN) { continue; }
        else if (cur_pkt_num >= base+RCV_WINDOW_LEN) {
            #ifdef DEBUG
            printf("ERROR: cur_pkt_num=%X, while base=%X\n", cur_pkt_num, base);
            #endif
            #ifdef GDB
            raise(SIGINT);
            #endif
            //continue;
            //break;
            memset((void*)cur_pkt, 0, PKT_SIZE);
            goto try_again;
        }
        else {
            pkt_sizes[cur_pkt_num-base] = cur_pkt[PKT_SIZE_IDX];
            memcpy(buf+((cur_pkt_num - base)*PKT_DATA_SIZE), cur_pkt+PKT_HEADER_SIZE, cur_pkt[PKT_SIZE_IDX]);
            buf_recv[cur_pkt_num - base] = 0x01;
            // Send ACK
            //memcpy((void*)ACK_pkt+SEQ_IDX, cur_pkt+SEQ_IDX, 2 * sizeof(uint8_t));
            #ifdef DEBUG
            printf("Sending ACK: %X\n", cur_pkt_num);
            #endif
            if (count % 2 == 0) {
                udp.send_data(&udp, cur_pkt, PKT_SIZE_IDX);
            }
            count++;
        }
        // Handler for when we can safely write from the buffer
        while ((write_count < RCV_WINDOW_LEN) && (buf_recv[write_count++] != 0)); // Count packets to be written
        write_count--;
        if (write_count > 0) {
            fwrite(buf, sizeof(char), (write_count-1)*PKT_DATA_SIZE+pkt_sizes[write_count-1], fp); // Write them to file
            // Move data up, overwriting written data.
            memmove(buf, &(buf[write_count*PKT_DATA_SIZE]),BUF_SIZE - write_count*PKT_DATA_SIZE);
            memmove(pkt_sizes, &(pkt_sizes[write_count]), RCV_WINDOW_LEN-write_count);
            memmove(buf_recv, &(buf_recv[write_count]), RCV_WINDOW_LEN - write_count);
            memset(&(buf_recv[RCV_WINDOW_LEN-write_count]), 0x00, write_count);
            base += write_count;
            #ifdef DEBUG
            printf("New base: %X\n", base);
            #endif
            write_count = 0;
        }
        //usleep(1000);
    } while (done == 0);

    fclose(fp);
    free(buf);
    // send FIN_ACK
    ACK_pkt[SEQ_IDX] = cur_pkt[SEQ_IDX];
    ACK_pkt[SEQ_IDX+1] = cur_pkt[SEQ_IDX+1];
    ACK_pkt[SEQ_IDX+2] = cur_pkt[SEQ_IDX+2];
    ACK_pkt[SEQ_IDX+3] = cur_pkt[SEQ_IDX+3];
    ACK_pkt[FLAG_IDX] = (ACK | FIN);
    printf("Sending FIN/ACK\n");
    udp.send_data(&udp, ACK_pkt, PKT_SIZE);
    shutdown(udp.socket, SHUT_WR);
    close(udp.socket);
    //udp.destroy(udp);
	printf("%s received.\n", destinationFile);
    free(ACK_pkt);
    free(pkt_sizes);
    free(cur_pkt);
    free(buf_recv);
    return;
}
/*
 * 
 */

int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
    return 0;
}

#endif
