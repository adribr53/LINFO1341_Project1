//
// Created by felix on 3/03/21.
//
#include <poll.h> // poll
#include <unistd.h> // read
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "read_write_loop_sender.h"
#include "../segment/packet_interface.h"

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
    uint32_t TIMEOUT = 100000000;
    uint8_t seqnum = 0;
    uint8_t startWindow = 0, endWindow = 0; // 1 ?
    uint8_t pktInWindow = 0;
    struct pkt* sendingWindow[31];
    for (int i = 0; i < 31; i++) {
        sendingWindow[i] = pkt_new();
        pkt_set_type(sendingWindow[i], 0);
    }

    uint8_t sendingWindowSize = 31;

    pkt_t *socketPkt = pkt_new();
    int socketResp;
    uint8_t receive_seqnum;
    // int waitForAck = FALSE;
    while (1) {
        int pollresp = poll(pfd, 2 , TIMEOUT);
        // if (pollresp == 0) printf("POLL EXPIRE\n");
        if (pfd[1].revents & POLLIN) { // stdin/fichier
            // fprintf(stderr, "Excuse moi but wtf ? \n");
            // read(stdin) -> write(socket)
            if (pktInWindow < sendingWindowSize) {
                nb=read(input_fd, payload_buffer, MAX_PAYLOAD_SIZE);
                if (nb != 0) {
                    pkt_set_type(socketPkt, 1);
                    pkt_set_tr(socketPkt, 0);
                    pkt_set_window(socketPkt, sendingWindowSize);
                    fprintf(stderr, "SEQNUM ENCODE >> %d\n", seqnum);
                    pkt_set_seqnum(socketPkt, seqnum);
                    pkt_set_length(socketPkt, nb);
                    pkt_set_timestamp(socketPkt, (uint32_t) clock()); // (set a la toute fin)
                    pkt_set_payload(socketPkt, payload_buffer, nb);
                    size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                    if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt\n"); return; } // encode pkt -> buf
                    // print_sent_pkt(socketPkt);
                    fprintf(stderr, "Paket N°%d has been sent\n", pkt_get_seqnum(socketPkt));
                    sendingWindow[endWindow] = socketPkt;
                    err=write(sfd, pkt_buffer, size); // sendto ???
                    // err = sendto(sfd, pkt_buffer, size, 0, (const struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
                    if (err<0) {fprintf(stderr, "Error while sendting the pkt\n"); return; }
                    endWindow = (endWindow+1)%31;
                    pktInWindow++;
                    seqnum++;
                    fprintf(stderr, "My supeeeeeeeeeeer number is %d\n", seqnum);
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
                switch ((int) pkt_get_type(socketPkt)) {
                    case 2:
                        // ACK

                        receive_seqnum = pkt_get_seqnum(socketPkt);
                        fprintf(stderr, "ACK N°%d", receive_seqnum);
                        // fprintf(stderr, "Brrrr\n");
                        i = startWindow;
                        while (receive_seqnum != pkt_get_seqnum(sendingWindow[i%31])) i++;
                        if (pkt_get_timestamp(socketPkt) == pkt_get_timestamp(sendingWindow[i%31])) {
                            uint8_t delta = i-startWindow+1;
                            startWindow = i%31;
                            pktInWindow -= delta;
                            sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;
                            fprintf(stderr,"\t paket left in window : %d\n", pktInWindow);
                        }
                        break;
                    case 3:
                        // NACK
                        sendingWindowSize = sendingWindowSize == 1 ? 1 : sendingWindowSize/2; // Resize sending window
                        i = 0;
                        while (pkt_get_seqnum(sendingWindow[i]) != pkt_get_seqnum(socketPkt)) i++; // maybe compute it later
                        pkt_set_timestamp(sendingWindow[i], (uint32_t) clock()); // update timestamp
                        size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                        if (pkt_encode(sendingWindow[endWindow], pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt NACK\n"); return; } // encode pkt -> buf
                        // print_sent_pkt(sendingWindow[endWindow]);
                        fprintf(stderr, "Paket N°%d has been resent (NACK resp)\n", pkt_get_seqnum(socketPkt));
                        err=write(sfd, pkt_buffer, size); // sendto ???
                        if (err<0) {fprintf(stderr, "Error while resinding pkt NACK\n"); return; }
                        break;
                }
            }
        }
        // Check RT
        uint8_t i = startWindow;
        while (i != endWindow) {
            if (pkt_get_type(sendingWindow[i]) == PTYPE_DATA && pkt_get_timestamp(sendingWindow[i]) + TIMEOUT < (uint32_t) clock()) {
                // RT expire => resend the pkt
                pkt_set_timestamp(sendingWindow[i], (uint32_t) clock()); // update timestamp
                size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                if (pkt_encode(sendingWindow[i], pkt_buffer, &size) != PKT_OK) {fprintf(stderr, "Error while encoding pkt RESEND\n"); return; } // encode pkt -> buf
                fprintf(stderr, "RESEND PAKET %d\n", pkt_get_seqnum(sendingWindow[i]));
                // print_sent_pkt(sendingWindow[i]);
                err=write(sfd, pkt_buffer, size); // sendto ???
                // err = sendto(sfd, pkt_buffer, size, 0, (const struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
                if (err<0) {
                    fprintf(stderr, "Value of errno: %d\n", errno);
                    fprintf(stderr, "Error opening file: %s\n", strerror( errno ));
                    return;
                }
            }
            i = (i + 1) % 31;
        }
    }
}