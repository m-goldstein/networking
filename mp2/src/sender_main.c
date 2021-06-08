#ifndef _SENDER_MAIN_C
#define _SENDER_MAIN_C
/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */
/*
 * Step 1: Create two arrays of window size of times.
 * Step 2: For each packet sent, set value in time array to time sent at (in us)
 * Step 3: For each packet (ACK) received, set value in recv'd time array to time recv'd (in us)
 * Step 4: At receive timeout, loop through rec'd time array, resend any unACK'd packets. 
 * Step 5: Whenever sliding window is moved, recalc average
 */
#include "utils.h"

struct sockaddr_in si_other;
struct sockaddr sa_dst;
int s, slen;

/*void diep(char *s) {
    perror(s);
    exit(1);
}*/


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    int rv;
    FILE *fp;
    struct stat file_stats;
    udp_t udp;
    struct timeval send_times[WINDOW_LEN];
    struct timeval recvd_times[WINDOW_LEN];
    //struct timeval avg_rtt;
    struct timeval rtt;
    //int pkts_sent = 0;
    //int pkts_recvd = 0;
    /* init udp connection */
    bzero((void*)udp.name, INET6_ADDRSTRLEN);
    strncpy((char*)udp.name, (char*)hostname, strlen(hostname));
    udp.port = hostUDPport;
    udp.p = NULL;
    udp.servinfo = NULL;
    udp.other = NULL;
    init_udp_send(&udp);
    uint32_t cur_pkt_num = 1;
    //Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    // Send SYN
    uint8_t recv_buf[WINDOW_LEN];
    uint8_t cur_pkt[PKT_SIZE];
    uint8_t rcv_pkt[PKT_SIZE];
    int nbytes = 0;
    memset(cur_pkt, 0, PKT_SIZE);
    cur_pkt[FLAG_IDX] |= SYN;
    printf("Sending SYN\n");
    do {
        memset(rcv_pkt, 0, PKT_SIZE);
        rv = udp.send_data(&udp, cur_pkt, PKT_SIZE);
        gettimeofday(&send_times[0], NULL); // handshake is always index 0
        if (rv == -1) {
            fprintf(stderr, "error sending packet.\n");
        }
        udp.recv_data(&udp, rcv_pkt, PKT_SIZE);
    } while ( (rcv_pkt[FLAG_IDX] & (SYN | ACK)) != (SYN | ACK)); 
    gettimeofday(&recvd_times[0], NULL); // handshake is always index 0
    timersub(&recvd_times[0], &send_times[0], &rtt);
    set_timer(udp.socket, rtt); // set timer
    printf("SYN/ACK recieved.\n");
    memset(cur_pkt, 0 , PKT_SIZE);
    cur_pkt[FLAG_IDX] |= ACK;
    printf("Sending ACK.\n");
    
    do {
        memset(rcv_pkt, 0, PKT_SIZE);
        rv = udp.send_data(&udp, cur_pkt, PKT_SIZE);
        if (rv == -1) {
            fprintf(stderr, "error sending packet.\n");
        }
        nbytes = udp.recv_data(&udp, rcv_pkt, PKT_SIZE);
    } while (nbytes > 0);
    //usleep(10000);
    printf("Handshake completed.\n");
    
    // file bookkeeping 
    if ( (rv = stat(filename, &file_stats)) < 0) {
        fprintf(stderr, "could not open file stats\n");
        return;
    }
    uint8_t* f_buf = (uint8_t*)malloc(sizeof(uint8_t)*BUF_SIZE);
    // actually send the file over
    int base = 0;
    int pkts_offset = WINDOW_LEN; // initially wipes slate clean 
    suseconds_t avg_in_microsec = convert_to_microsec(rtt);
    //uint32_t send_wait = convert_to_microsec(rtt);
    suseconds_t avg_tv_diffs[TIMER_AVG_SIZE]; // arr of average time diffs
    int avg_tv_idx = 0;
    bzero(recv_buf, WINDOW_LEN);
    long long int bytes_left = (long long int)bytesToTransfer;
    long long int bytes_read = 0;
    long long int total_bytes_read = 0;
    while (bytes_left > 0) {
        long long int packetsToSend;
        if (bytes_left < BUF_SIZE) {
            packetsToSend = bytes_left/PKT_DATA_SIZE;
            if (bytes_left%PKT_DATA_SIZE) { packetsToSend++; }
        }
        else {
            packetsToSend = WINDOW_LEN;
        }
        /* 
         * 1. fread data into new buffer and make space for 
         *    timevals and data to write.
         * 2. memset recv_buf (ACKs) to 0 starting at first 
         *    unACKd packet
         */
        // clear first packet that hasn't been shifted 
        memset((void*)send_times+(WINDOW_LEN-pkts_offset), 0, (pkts_offset) * sizeof(struct timeval));
        memset((void*)recvd_times+(WINDOW_LEN-pkts_offset), 0, (pkts_offset) * sizeof(struct timeval));
        memset((void*)recv_buf+(WINDOW_LEN-pkts_offset), 0, pkts_offset+1);
        bytes_read = fread(f_buf + BUF_SIZE - (pkts_offset * PKT_DATA_SIZE), 1, pkts_offset * PKT_DATA_SIZE, fp);
        total_bytes_read += bytes_read;
        //int end_of_file = 0;
        //if (bytes_read < pkts_offset*PKT_DATA_SIZE) { end_of_file = 1; }
        /*
         * 3. Loop over window. Send every packet without an ACK 
         *    timestamp
         */
        for (long long int i = 0; i < packetsToSend; i++) {
            if (recv_buf[i] == 0x0 ) {
                cur_pkt_num = base+i;
                bzero(cur_pkt, PKT_SIZE);
                cur_pkt[SEQ_IDX] = (cur_pkt_num >> 24) & 0xff; 
                cur_pkt[SEQ_IDX+1] = (cur_pkt_num >> 16) & 0xff; 
                cur_pkt[SEQ_IDX+2] = (cur_pkt_num >> 8) & 0xff; 
                cur_pkt[SEQ_IDX+3] = (cur_pkt_num) & 0xff;    
                
                int pkt_len = PKT_DATA_SIZE;
                if (packetsToSend != WINDOW_LEN && i == packetsToSend-1) {
                    pkt_len = bytes_left-(i*PKT_DATA_SIZE);
                }
                cur_pkt[PKT_SIZE_IDX] = pkt_len;
                memcpy(cur_pkt + PKT_HEADER_SIZE, f_buf + (i * PKT_DATA_SIZE), pkt_len);
                rv = udp.send_data(&udp, cur_pkt, PKT_HEADER_SIZE+pkt_len); 
                if (rv == -1) {
                    fprintf(stderr, "error sending packet.\n");
                }
                gettimeofday(&send_times[i], NULL);
                #ifdef DEBUG
                printf("Sent packet %X\n", cur_pkt_num);
                #endif
                //usleep(send_wait/10);
                //break;
            }
        }
        /*
         *  4. recv packets containing ACK from receiver. Update bufs
         *  and timers as needed.
         */
        float alpha = 0.125f;
        float beta = 0.25f;
        suseconds_t tmp;
        suseconds_t timeshift;
        suseconds_t estimate = convert_to_microsec(rtt);
        suseconds_t deviation = 0;
        struct timeval cur_diff;
        bzero(cur_pkt, PKT_SIZE);
        while ((rv = udp.recv_data(&udp, cur_pkt, PKT_SIZE_IDX)) > 0) {
            //cur_pkt_num = ((0xFF&cur_pkt[SEQ_IDX]) << 8) | (0xFF&cur_pkt[SEQ_IDX+1]);
            cur_pkt_num = 0x0;
            cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX]) & 0xff; 
            cur_pkt_num <<= 8;
            cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+1]) & 0xff;
            cur_pkt_num <<= 8;
            cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+2]) & 0xff;
            cur_pkt_num <<= 8;
            cur_pkt_num |= ((uint32_t)cur_pkt[SEQ_IDX+3]) & 0xff;
            #ifdef DEBUG
            printf("Recieved an ACK: %X\n", cur_pkt_num);
            #endif
            int pkt_idx = cur_pkt_num-base;
            if (pkt_idx >= 0 && pkt_idx < WINDOW_LEN) {
                if (recv_buf[pkt_idx] == 0) {
                    #ifdef DEBUG
                    printf("   ACK was new\n");
                    #endif
                    recv_buf[pkt_idx] = 0x1;
                    gettimeofday(&recvd_times[pkt_idx], NULL);
                    if (base >= TIMER_AVG_SIZE) { 
                        timersub(&recvd_times[pkt_idx], &send_times[pkt_idx], &cur_diff);
                        // convert current avg to uS
                        tmp = convert_to_microsec(cur_diff);
                        timeshift = avg_tv_diffs[avg_tv_idx];
                        avg_in_microsec = (suseconds_t)(TIMER_AVG_SIZE * avg_in_microsec - timeshift + tmp) / TIMER_AVG_SIZE;
                        avg_tv_diffs[avg_tv_idx] = tmp;
                        avg_tv_idx = (avg_tv_idx + 1)  % TIMER_AVG_SIZE;
                    } else {
                        timersub(&recvd_times[pkt_idx], &send_times[pkt_idx], &cur_diff);
                        // convert current avg to uS
                        tmp = convert_to_microsec(cur_diff);
                        //   add newest timediff
                        avg_in_microsec = (avg_tv_idx * avg_in_microsec + tmp) / (avg_tv_idx + 1);
                        // convert avg to timeval
                        avg_tv_diffs[avg_tv_idx] = tmp;
                        avg_tv_idx = (avg_tv_idx + 1)  % TIMER_AVG_SIZE;
                    }
                    if (avg_in_microsec < 0 || avg_in_microsec > 2000000) {
                        avg_in_microsec = 25000;
                    }
                    estimate = (1-alpha) * (1.0*estimate) + (1.0*alpha) * avg_in_microsec;
                    suseconds_t delta;
                    switch (avg_in_microsec - estimate > 0) {
                        case 1: 
                            delta = avg_in_microsec - estimate;
                            break;
                        default:
                            delta = estimate - avg_in_microsec;
                            break;
                    }
                    deviation = (1.0-beta) * (1.0*deviation) + (1.0*beta) * (delta);
                    //if (tmp < 0) { printf("NEGATIVE TMP: %i\n", tmp); }
                    //if (avg_in_microsec < 0) { 
                    //    avg_in_microsec = 250000;
                        //printf("NEG AVG TIME: %i\n", avg_in_microsec); 
                    //}

                }

            }
            //usleep(avg_in_microsec);
            //if (pkt_idx == 0) { break; }
        } 
        /* Convert average time in microsec back to timeval and set timer on socket */
        //if (avg_in_microsec > 2000000) { avg_in_microsec = 2000; }
        //avg_in_microsec = (suseconds_t)((1.0 - alpha)*avg_in_microsec*1.0);
        suseconds_t timeout = estimate + 4.0 * deviation;
        if (timeout < 0) {
            timeout = 25000;
        }
        timeout |= 0x1;
        rtt.tv_sec = (time_t)timeout / 1000000;
        rtt.tv_usec = timeout % 1000000;
        #ifdef DEBUG
        printf("timeout: %i\n", timeout);
        #endif
        //usleep(avg_in_microsec%1000000);
        set_timer(udp.socket, rtt);
        /*
         * 5. Loop through window, count how many contiguous packets
         *    received an ACK. Break as soon as unACK'd packet is 
         *    encountered.
         */
        pkts_offset = 0;
        while (pkts_offset < WINDOW_LEN && recv_buf[pkts_offset++] == 1);
        pkts_offset--;
        /*
         * 6. Shift sliding window up to first unACK'd pkt. 
         */
        memmove(f_buf, f_buf + (pkts_offset * PKT_DATA_SIZE), BUF_SIZE - (pkts_offset * PKT_DATA_SIZE));
        memmove(send_times, send_times + pkts_offset, WINDOW_LEN - pkts_offset);
        memmove(recvd_times, recvd_times + pkts_offset, WINDOW_LEN - pkts_offset);
        memmove(recv_buf, recv_buf + pkts_offset, WINDOW_LEN - pkts_offset); 
        base += pkts_offset;
        /*printf("pkts_offset: %X\n", pkts_offset);
        printf("New base: %X\n", base);
        printf("ack at new base:%X\n", recv_buf[0]);*/
        bytes_left -= (pkts_offset*PKT_DATA_SIZE);
        #ifdef PROGRESS
        printf("\r%.2f%% complete\n", (float)100*(1-((1.0*bytes_left)/bytesToTransfer)));
        fflush(stdout);
        #endif
        //usleep(100000);
    }

    memset(cur_pkt, 0, PKT_SIZE);
    cur_pkt[FLAG_IDX] |= FIN;
    printf("Sending FIN\n");
    do {
        memset(rcv_pkt, 0, PKT_SIZE);
        rv = udp.send_data(&udp, cur_pkt, PKT_SIZE);
        if (rv == -1) {
            fprintf(stderr, "error sending packet.\n");
        }
        udp.recv_data(&udp, rcv_pkt, PKT_SIZE);
    } while ( (rcv_pkt[FLAG_IDX] & (FIN | ACK)) != (FIN | ACK)); 
    printf("Recieved FIN/ACK\n");

    memset(cur_pkt, 0, PKT_SIZE);
    cur_pkt[FLAG_IDX] |= ACK;
    printf("Sending ACK\n");
    rv = udp.send_data(&udp, cur_pkt, PKT_SIZE);
    if (rv == -1) {
        fprintf(stderr, "error sending packet.\n");
    }
    printf("Sent ACK to client.\n");
    printf("Sent %lld\n bytes to client\n", total_bytes_read);
    printf("Closing the socket\n");
    shutdown(udp.socket, SHUT_WR);
    close(udp.socket);
    free(f_buf);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}
#endif
