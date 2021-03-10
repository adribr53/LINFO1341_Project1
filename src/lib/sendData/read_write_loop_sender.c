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

#define TRUE 1
#define FALSE 0

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
    uint8_t startWindow = 0, endWindow = 0; // 1 ?
    uint8_t pktInWindow = 0;

    linkedList window = new_list();

    uint8_t sendingWindowSize = 1;

    pkt_t *socketPkt = pkt_new();
    pkt_t *tmpPtk = pkt_new();
    int socketResp;
    uint8_t receive_seqnum;

    uint32_t clock_time;

    // int waitForAck = FALSE;
    while (1) {
        int pollresp = poll(pfd, 2 , TIMEOUT);

        // fprintf(stderr, "POLLRESP >> %d, [STDIN] > %d, [SKT] > %d\n", pollresp, pfd[1].revents, pfd[0].revents);
        if (pollresp == 0) fprintf(stderr, "POLL EXPIRE\n");
        if (pfd[1].revents & POLLIN) {
            // fprintf(stderr, "Excuse moi but wtf ? \n");
            // read(stdin) -> write(socket)
            if (pktInWindow <= sendingWindowSize) {
                nb=read(input_fd, payload_buffer, MAX_PAYLOAD_SIZE);
                if (nb != 0) {
                    pkt_set_type(socketPkt, 1);
                    pkt_set_tr(socketPkt, 0);
                    pkt_set_window(socketPkt, sendingWindowSize);
                    // fprintf(stderr, "SEQNUM ENCODE >> %d\n", seqnum);
                    pkt_set_seqnum(socketPkt, seqnum);
                    pkt_set_length(socketPkt, nb);
                    pkt_set_timestamp(socketPkt, (uint32_t) clock()); // (set a la toute fin)
                    pkt_set_payload(socketPkt, payload_buffer, nb);
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt\n"); return; } // encode pkt -> buf
                    // print_sent_pkt(socketPkt);
                    fprintf(stderr, "Paket N°%d has been sent\n", pkt_get_seqnum(socketPkt));
                    if (add(window, socketPkt) < 0) {fprintf(stderr, "Error with adding in window\n"); return;}
                    // err=write(sfd, pkt_buffer, size); // sendto ???
                        err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                    // err = sendto(sfd, pkt_buffer, size, 0, (const struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
                    if (err<0) {fprintf(stderr, "Error while sendting the pkt\n"); return; }
                    endWindow = (endWindow+1)%31;
                    pktInWindow++;
                    seqnum++;
                    sentPkt++;
                    // fprintf(stderr, "My supeeeeeeeeeeer number is %d\n", seqnum);
                } else {
                    if (sendingWindowSize == 0) {fprintf(stderr, "All data has been sent\n"); return; } // all data has been sent
                    // envoi d'un paquet terminal(data, length=0)
                    // quand on reçoit le num de seq relatif : le programme est terminé
                }
            }
            // waitForAck = TRUE;
        }

        if (pfd[0].revents & POLLIN) {
            // read(socket) -> write(stdout)
            nb=read(sfd, pkt_buffer, 12);

            socketResp = pkt_decode(pkt_buffer, 12, socketPkt);
            if (socketResp == PKT_OK) { // if pkt not ok we just drop it
                uint8_t i;
                receive_seqnum = pkt_get_seqnum(socketPkt);
                switch ((int) pkt_get_type(socketPkt)) {
                    case 2:
                        // ACK
                        fprintf(stderr, "ACK N°%d\n", receive_seqnum);
                        if (get(window, receive_seqnum, tmpPtk) == 0) {
                            if (pkt_get_timestamp(socketPkt) == pkt_get_timestamp(tmpPtk)) {
                                while (pkt_get_seqnum(pop(window)) != receive_seqnum) {startWindow++; pktInWindow--;}
                                pktInWindow--;
                                sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;

                            }
                        }
                        break;
                    case 3:
                        // NACK
                        sendingWindowSize = sendingWindowSize == 1 ? 1 : sendingWindowSize/2; // Resize sending window
                        if (get(window, receive_seqnum, tmpPtk) == 0) {
                            pkt_get_timestamp(tmpPtk, (uint32_t) clock());
                            // resend
                            size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                            if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt NACK\n"); return; } // encode pkt -> buf
                            fprintf(stderr, "Paket N°%d has been resent (NACK resp)\n", pkt_get_seqnum(tmpPtk));
                            err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                            if (err<0) {fprintf(stderr, "Error while resinding pkt NACK\n"); return; }
                        }
                        break;
                }
            }
        }
        // Check RT
        clock_time = (uint32_t) clock();
        tmpPtk = peek(window);
        if (tmpPtk != NULL) {
            if (pkt_get_timestamp(tmpPtk) + TIMEOUT < clock_time) {
                pkt_get_timestamp(tmpPtk, (uint32_t) clock());
                // resend
                size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                if (pkt_encode(tmpPtk, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt RESEND\n"); return; } // encode pkt -> buf
                fprintf(stderr, "Paket N°%d has been resent (RESEND)\n", pkt_get_seqnum(tmpPtk));
                err=send(sfd, pkt_buffer, size, MSG_CONFIRM);
                if (err<0) {fprintf(stderr, "Error while RESEND\n"); return; }
            }
        }
        if (nb == 0 && pktInWindow == 0) {
            fprintf(stderr, "ALL PKT SENT (%d)\n", sentPkt);
            return;
        }
    }
}