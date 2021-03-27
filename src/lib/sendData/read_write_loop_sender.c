//
// Created by felix on 3/03/21.
//
#include <poll.h> // poll
#include <unistd.h> // read
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "read_write_loop_sender.h"
#include "../segment/packet_interface.h"
#include "../queue/queue.h"
#include "../stats/stat.h"


long long create_first_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);                                         // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
    return milliseconds;
}

uint32_t current_timestamp(long long start) {
    // adaptation from https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
    struct timeval te;
    gettimeofday(&te, NULL);                                         // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
    return (uint32_t) milliseconds - start;
}
/*
 * Build a pkt
 * pkt: buffer where the pake will be written
 * windowSize : current size of the window
 * seqnum : sequence number of the pkt
 * nb : lenght of the payload (max 512)
 * payload : buffer to put on the payload of the paket (should have a size of nb)
 */
void build_pkt(pkt_t* pkt, uint8_t windowSize, uint8_t seqnum, int nb, char* payload, uint32_t timestamp) {
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, windowSize);
    pkt_set_seqnum(pkt, seqnum);
    pkt_set_length(pkt, nb);
    pkt_set_timestamp(pkt, timestamp);
    pkt_set_payload(pkt, payload, nb);
}

/*
 * Sending lopp
 * sfd : the socket file descriptor
 * input_fd : the input file
 */
void read_write_loop_sender(const int sfd, const int input_fd, FILE* stats_file) {
    
    // Init variables

    struct pollfd pfd[2];
    // 0 => socket
    // 1 => sender input

    struct pollfd sockPoll;
    sockPoll.fd=sfd;
    sockPoll.events=POLLIN;

    struct pollfd inputPoll;
    inputPoll.fd=input_fd;
    inputPoll.events=POLLIN;

    pfd[0]=sockPoll;
    pfd[1]=inputPoll;

    char payload_buffer[MAX_PAYLOAD_SIZE];
    char pkt_buffer[MAX_PAYLOAD_SIZE+16];
    int nb = 0; // variable utilisée avec write()/read()
    int err;
    int sentPkt = 0; // booleen

    uint32_t TIMEOUT = 1200;
    uint8_t seqnum = 0;
    uint8_t pktInWindow = 0;

    queue* window = new_list();
    uint8_t sendingWindowSize = 1; // devrait être 1

    pkt_t *socketPkt;
    pkt_t *tmpPtk;

    int socketResp;

    uint8_t receive_seqnum;
    uint32_t clock_time;

    long long startingTimestamp = create_first_timestamp();


    //int rtSupport=0;
    int *supportInt=malloc(sizeof(int));
    *supportInt=-1;

    logger* sender_logger = new_logger(1, stats_file);

    // Starting loop
    while (1) {
        int pollresp = poll(pfd, 2 , TIMEOUT);
        if (pollresp == -1) {fprintf(stderr, "Error while poll\n"); return;}
        if (pfd[1].revents & POLLIN) {
            // Event on input_fd
            // last ack received + window size
            if (pktInWindow < sendingWindowSize) {
                nb=read(input_fd, payload_buffer, MAX_PAYLOAD_SIZE);
                if (nb != 0) {
                    // Build the pkt
                    socketPkt = pkt_new();
                    build_pkt(socketPkt, sendingWindowSize-pktInWindow, seqnum, nb, payload_buffer, current_timestamp(startingTimestamp));
                    // Encode the pkt in the buffer
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) {
                        fprintf(stderr, "Error while encoding pkt\n"); 
                        return; 
                    } // encode pkt -> buf
                    fprintf(stderr, "Paket N°%d has been sent\n", pkt_get_seqnum(socketPkt));
                    // Add the pkt to the sending window
                    if (add(window, socketPkt) < 0) {
                        fprintf(stderr, "Error with adding in window\n"); 
                        return;
                    }
                    // Send the pkt
                    err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                    if (err<0) {fprintf(stderr, "Error while sendting the pkt\n"); return; }
                    add_data_sent(sender_logger);
                    // Update variables
                    pktInWindow++;
                    seqnum=(seqnum+1)%256; // prochain numero de sequence
                    sentPkt=1;
                }
            }
        }

        if (pfd[0].revents & POLLIN) {
            // Event on socket
            nb=read(sfd, pkt_buffer, 12);
            // Decode the pkt
            socketPkt = pkt_new();
            socketResp = pkt_decode(pkt_buffer, 12, socketPkt);
            if (socketResp == PKT_OK) { // if pkt not ok we just drop it
                if (pkt_get_tr(socketPkt) != 1) {
                    receive_seqnum = pkt_get_seqnum(socketPkt);
                    receive_seqnum = (receive_seqnum!=0) ? receive_seqnum-1 : 255;
                    switch ((int) pkt_get_type(socketPkt)) {
                        case 1:
                            // DATA => ignored
                            add_packet_duplicated(sender_logger);
                            add_data_received(sender_logger);
                            break;
                        case 2:
                            // ACK
                            fprintf(stderr, "Received Ack : %d\n", receive_seqnum);
                            add_ack_received(sender_logger);

                            uint8_t isChanged = 0;
                            // Remove all validate ack (if first sent is 1 and receive ack is 3 -> 1, 2 and 3 are ack)
                            while (is_higher_or_equal(receive_seqnum, window))
                            {
                                dequeue(window);
                                pktInWindow--;
                                isChanged = 1;
                            }
                            if (isChanged == 0) {
                                // ignored
                                add_packet_ignored(sender_logger);
                            } else {
                                new_rtt(sender_logger, current_timestamp(startingTimestamp) - pkt_get_timestamp(socketPkt));
                            }
                            //sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;
                            sendingWindowSize=pkt_get_window(socketPkt);
                            break;
                        case 3:
                            // NACK
                            *supportInt=-1; // support pour itérer sur get (utile plus loin, pas pour ce bloc)
                            add_nack_received(sender_logger);
                            if (get(window, receive_seqnum, &tmpPtk, supportInt)) {
                                pkt_set_timestamp(tmpPtk, current_timestamp(startingTimestamp));
                                // Resend the pkt
                                size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                                if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt NACK\n"); return; } // encode pkt -> buf
                                fprintf(stderr, "Paket N°%d has been resent (NACK   )\n", pkt_get_seqnum(tmpPtk));

                                err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                                if (err<0) {fprintf(stderr, "Error while resinding pkt NACK\n"); return; }
                                add_packet_retransmitted(sender_logger);

                                sendingWindowSize = sendingWindowSize == 1 ? 1 : sendingWindowSize/2; // Resize sending window
                            } else {
                                // ignored
                                add_packet_ignored(sender_logger);
                            }
                            break;
                    }
                } else {
                    // pkt truncated
                    add_data_truncated_received(sender_logger);
                }
                pkt_del(socketPkt);
            } else {
                // ignored packet
                add_packet_ignored(sender_logger);
            }
        }
        // Check RT
        clock_time = current_timestamp(startingTimestamp);
        tmpPtk = peek(window);
        if (tmpPtk != NULL) {
            receive_seqnum = pkt_get_seqnum(tmpPtk);
            *supportInt=-1; // outil pour itérer
            uint8_t first=pkt_get_seqnum(peek(window));
            int firstTime=1;
            while (get(window, receive_seqnum, &tmpPtk, supportInt)) {
                if (first==pkt_get_seqnum(tmpPtk) && !firstTime) {
                    break;
                }
                if (pkt_get_timestamp(tmpPtk) + TIMEOUT/20 < clock_time) {
                    // Set new timestamp
                    pkt_set_timestamp(tmpPtk, current_timestamp(startingTimestamp));
                    // resend
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt RESEND\n"); return; } // encode pkt -> buf
                    err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                    if (err<0) {fprintf(stderr, "Error while RESEND\n"); return; }
                    add_packet_retransmitted(sender_logger);
                }
                receive_seqnum=(receive_seqnum+1)%MAX_WINDOW_SIZE;
                firstTime=0;
            }
        }

        if (nb == 0 && pktInWindow == 0) {
            // Send pkt with empty payload to close the receiver (if lost the receiver will automaticly stop with a timeout)
            socketPkt = pkt_new();
            build_pkt(socketPkt, sendingWindowSize, seqnum, 0, payload_buffer, current_timestamp(startingTimestamp)); // payload_buffer not null, and unused
            // Encode the pkt in the buffer
            size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
            if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) {
                fprintf(stderr, "Error while encoding pkt\n"); 
                return; 
            } // encode pkt -> buf
            // Send the pkt
            err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
            if (err<0) {fprintf(stderr, "Error while sendting the pkt\n");}
            add_data_sent(sender_logger);

            fprintf(stderr, "sender terminates\n");
            fprintf(stderr, "ALL PKT SENT (%d)\n", sentPkt);
            // Free struc
            del_list(window);
            if (socketPkt != NULL) pkt_del(socketPkt);
            if (tmpPtk != NULL) pkt_del(tmpPtk);
            free(supportInt);

            remove_logger(sender_logger); // write in file if needed
            return;
        }
    }
}