//
// Created by felix on 3/03/21.
//
#include <poll.h> // poll
#include <unistd.h> // read
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "read_write_loop_sender.h"
#include "../segment/packet_interface.h"
#include "../linkedlist/linkedlist.h"


void print_sent_pkt(pkt_t* paket) {
    fprintf(stderr, "=== PAKET SENT ===\n");
    fprintf(stderr, "type => %d\n", pkt_get_type(paket));
    fprintf(stderr, "TR => %d\n", pkt_get_tr(paket));
    fprintf(stderr, "WINDOW => %d\n", pkt_get_window(paket));
    fprintf(stderr, "Length => %d\n", pkt_get_length(paket));
    fprintf(stderr, "Seqnum => %d\n", pkt_get_seqnum(paket));
    fprintf(stderr, "timestamp => %d\n", pkt_get_timestamp(paket));
    fprintf(stderr, "crc1 => %d\n", pkt_get_crc1(paket));
    fprintf(stderr, "Payload => %s\n", pkt_get_payload(paket));
    fprintf(stderr, "crc2 => %d\n", pkt_get_crc2(paket));
    fprintf(stderr, "=== BRRRRRRR ===\n\n");
}

int build_pkt(pkt_t* pkt, uint8_t windowSize, uint8_t seqnum, int nb, char* payload) {
    pkt_set_type(pkt, 1);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, windowSize);
    pkt_set_seqnum(pkt, seqnum);
    pkt_set_length(pkt, nb);
    pkt_set_timestamp(pkt, (uint32_t) clock());
    pkt_set_payload(pkt, payload, nb);
}

void read_write_loop_sender(const int sfd, const int input_fd) {
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
    int nb;
    int err;
    int sentPkt = 0;

    uint32_t TIMEOUT = 12000;
    uint8_t seqnum = 0;
    uint8_t pktInWindow = 0;

    linkedList* window = new_list();
    uint8_t sendingWindowSize = 4;

    pkt_t *socketPkt;
    pkt_t *tmpPtk;

    int socketResp;

    uint8_t receive_seqnum;
    uint32_t clock_time;

    while (1) {
        int pollresp = poll(pfd, 2 , TIMEOUT);
        if (pfd[1].revents & POLLIN) {
            // Event on input_fd
            if (pktInWindow <= sendingWindowSize) {
                nb=read(input_fd, payload_buffer, MAX_PAYLOAD_SIZE);
                if (nb != 0) {
                    // Build the pkt
                    socketPkt = pkt_new();
                    build_pkt(socketPkt, sendingWindowSize, seqnum, nb, payload_buffer);
                    // Encode the pkt in the buffer
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt\n"); return; } // encode pkt -> buf
                    fprintf(stderr, "Paket N°%d has been sent\n", pkt_get_seqnum(socketPkt));
                    // Add the pkt to the sending window
                    if (add(window, socketPkt) < 0) {fprintf(stderr, "Error with adding in window\n"); return;}
                    // Send the pkt
                    err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                    if (err<0) {fprintf(stderr, "Error while sendting the pkt\n"); return; }
                    // Update variables
                    pktInWindow++;
                    seqnum++;
                    sentPkt++;
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
                uint8_t i;
                receive_seqnum = pkt_get_seqnum(socketPkt);
                switch ((int) pkt_get_type(socketPkt)) {
                    case 2:
                        // ACK
                        if (get(window, receive_seqnum, &tmpPtk) == 0) {
                            fprintf(stderr, "Ack N°%d validate\n", receive_seqnum);
                            // Remove pkt until the receive_seqnum
                            while (pkt_get_seqnum(pop(window)) != receive_seqnum) {pktInWindow--;}
                            pktInWindow--;
                            sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;
                        }
                        break;
                    case 3:
                        // NACK
                        if (get(window, receive_seqnum, &tmpPtk) == 0) {
                            pkt_set_timestamp(tmpPtk, (uint32_t) clock());
                            // Resend the pkt
                            size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                            if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt NACK\n"); return; } // encode pkt -> buf
                            fprintf(stderr, "Paket N°%d has been resent (NACK resp)\n", pkt_get_seqnum(tmpPtk));

                            err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                            if (err<0) {fprintf(stderr, "Error while resinding pkt NACK\n"); return; }

                            sendingWindowSize = sendingWindowSize == 1 ? 1 : sendingWindowSize/2; // Resize sending window
                        }
                        break;
                }
            }
        }
        // Check RT
        clock_time = (uint32_t) clock();
        tmpPtk = peek(window);
        if (tmpPtk != NULL) {
            receive_seqnum = pkt_get_seqnum(tmpPtk);
            while (get(window, receive_seqnum, &tmpPtk) == 0) {
                if (pkt_get_timestamp(tmpPtk) + TIMEOUT < clock_time) {
                    // Set new timestamp
                    pkt_set_timestamp(tmpPtk, (uint32_t) clock());
                    // resend
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt RESEND\n"); return; } // encode pkt -> buf
                    fprintf(stderr, "Paket N°%d has been resent (RESEND)\n", pkt_get_seqnum (tmpPtk));

                    err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                    if (err<0) {fprintf(stderr, "Error while RESEND\n"); return; }
                }
                receive_seqnum++;
            }
        }

        if (nb == 0 && pktInWindow == 0) {
            fprintf(stderr, "ALL PKT SENT (%d)\n", sentPkt);
            return;
        }
    }
}