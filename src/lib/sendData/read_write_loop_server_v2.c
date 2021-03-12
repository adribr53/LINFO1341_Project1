//
// Created by felix on 12/03/21.
//

#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>
#include "../sortedLinkedList/sortedLinkedList.h"

#define ACK_PKT_SIZE 10

int in_window(uint8_t waited, uint8_t received) {
    if (received > waited && received - waited <= MAX_WINDOW_SIZE) return 1;
    if (received < waited + MAX_WINDOW_SIZE && waited > waited + MAX_WINDOW_SIZE) return 1;
    return 0;
}

int build_ack(pkt_t* pkt, pkt_t* data_pkt, uint8_t seqnum) {
    pkt_set_type(pkt, PTYPE_ACK);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, MAX_WINDOW_SIZE);
    pkt_set_seqnum(pkt, seqnum);
    pkt_set_length(pkt, 0);
    pkt_set_timestamp(pkt, pkt_get_timestamp(data_pkt));
    return 0;
}

void read_write_loop_server(const int sfd, const int outfd) {
    struct pollfd sfdPoll;
    sfdPoll.fd=sfd;
    sfdPoll.events=POLLIN;

    list_t* window=new_list();
    size_t ack_size = (size_t) 10;
    char data_buffer[MAX_PAYLOAD_SIZE+16];
    int nb;
    char buffer[ACK_PKT_SIZE];
    uint8_t waitedSeqnum = 0; // uint8 so no need to take care of 0-1 and 255+1
    uint64_t receivePkt = 0;
    while (1) {
        poll(&sfdPoll, 1 , -1);
        if (sfdPoll.revents && POLLIN) {
            nb=recv(sfd, data_buffer, MAX_PAYLOAD_SIZE+16, 0);
            if (nb==0) return;
            // Decode pkt
            pkt_t *pkt=pkt_new();
            if (pkt_decode(data_buffer, nb, pkt) == PKT_OK) {
                if (pkt_get_type(pkt) == PTYPE_DATA) {
                    // Data
                    if (pkt_get_seqnum(pkt) == waitedSeqnum) {
                        // parse window
                        nb=write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
                        if (nb == 0) {fprintf(stderr, "Error while writing in outfile\n"); return;}
                        waitedSeqnum++;
                        receivePkt++;
                        // Unfill the window with consecutive seqnum
                        while (peek(window) != NULL && waitedSeqnum == pkt_get_seqnum(peek(window))) {
                            pkt = pop(window);
                            nb=write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
                            if (nb == 0) {fprintf(stderr, "Error while writing in outfile\n"); return;}
                            waitedSeqnum++; // modulo 256
                            receivePkt++;
                        }
                        // Send ack
                        pkt_t* ack_pkt = pkt_new();
                        build_ack(ack_pkt, pkt, pkt_get_seqnum(pkt));
                        pkt_encode(ack_pkt, buffer, &ack_size);
                        nb = send(sfd, buffer, ack_size, MSG_CONFIRM);
                        if (nb == 0) {
                            fprintf(stderr, "Error while sending ACK\n");
                            return;
                        }
                        pkt_del(ack_pkt);
                    } else { // Unwaited seqnum
                        if (in_window(waitedSeqnum, pkt_get_seqnum(pkt))) {
                            // Add pkt to the received window
                            add(window, pkt);
                            if (receivePkt != 0) {
                                // Resend last pkt ack
                                pkt_t* ack_pkt = pkt_new();
                                build_ack(ack_pkt, pkt, waitedSeqnum-1);
                                pkt_encode(ack_pkt, buffer, &ack_size);
                                nb = send(sfd, buffer, ack_size, MSG_CONFIRM);
                                if (nb == 0) {
                                    fprintf(stderr, "Error while resending ACK\n");
                                    return;
                                }
                                pkt_del(ack_pkt);
                            }
                        }
                    }
                }
            }
        }
    }
}