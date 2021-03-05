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

int is_in_window_interval(uint8_t seqnum, uint8_t start, uint8_t end) {
    if (seqnum > 31) fprintf(stderr, "WTF ???\n");
    if (start < end) {
        return (start <= seqnum) && (seqnum <= end);
    } else {
        return (start <= seqnum) && (seqnum <= end);
    }
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
    uint8_t startWindow = 0, endWindow = 0;
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
        if (pfd[1].revents & POLLIN) {
            // fprintf(stderr, "Excuse moi but wtf ? \n");
            // read(stdin) -> write(socket)
            if (pktInWindow < sendingWindowSize) {
                nb=read(input_fd, payload_buffer, MAX_PAYLOAD_SIZE);
                if (nb==0) return; // all data has been sent
                pkt_set_type(socketPkt, 1);
                pkt_set_tr(socketPkt, 0);
                pkt_set_window(socketPkt, sendingWindowSize);
                pkt_set_seqnum(socketPkt, seqnum);
                pkt_set_length(socketPkt, nb);
                pkt_set_timestamp(socketPkt, (uint32_t) clock());
                pkt_set_payload(socketPkt, payload_buffer, nb);
                size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                if (pkt_encode(socketPkt, pkt_buffer, &size) != PKT_OK) return; // encode pkt -> buf
                print_sent_pkt(socketPkt);
                sendingWindow[endWindow] = socketPkt;
                err=write(sfd, pkt_buffer, size); // sendto ???
                // err = sendto(sfd, pkt_buffer, size, 0, (const struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
                if (err<0) return;

                endWindow = (endWindow+1)%31;
                pktInWindow++;
                seqnum++;
                fprintf(stderr, "SEQNUM ENCULE%d\n", seqnum);
            } else {
                fprintf(stderr, "NTM\n");
            }
            // waitForAck = TRUE;
        }

        if (pfd[0].revents & POLLIN) {
            // read(socket) -> write(stdout)
            fprintf(stderr, "COUCOU LA MIF\n");
            nb=read(sfd, pkt_buffer, 12);

            socketResp = pkt_decode(pkt_buffer, 12, socketPkt);
            if (socketResp == PKT_OK) { // if pkt not ok we just drop it
                fprintf(stderr, "Hello there !\n");
                switch ((int) pkt_get_type(socketPkt)) {
                    case 2:
                        // ACK
                        receive_seqnum = pkt_get_seqnum(socketPkt);
                        fprintf(stderr, "ACK N°%d\n", receive_seqnum);
                        if (is_in_window_interval(receive_seqnum, startWindow, endWindow)) { // pkt stored in receiver buffer or duplicate
                            uint8_t i = startWindow;
                            while (receive_seqnum != pkt_get_seqnum(sendingWindow[i%31])) i++;
                            uint8_t delta = i-startWindow+1;
                            startWindow = i%31;
                            pktInWindow -= delta;
                            sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;
                        }
                        break;
                    case 3:
                        // NACK
                        sendingWindowSize = sendingWindowSize == 1 ? 1 : sendingWindowSize/2; // Resize sending window
                        int i = 0;
                        while (pkt_get_seqnum(sendingWindow[i]) != pkt_get_seqnum(socketPkt)) i++; // maybe compute it later
                        pkt_set_timestamp(sendingWindow[i], (uint32_t) clock()); // update timestamp
                        size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                        if (pkt_encode(sendingWindow[endWindow], pkt_buffer, &size) != PKT_OK) return; // encode pkt -> buf
                        print_sent_pkt(sendingWindow[endWindow]);
                        err=write(sfd, pkt_buffer, size); // sendto ???
                        if (err<0) return;
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
                if (pkt_encode(sendingWindow[i], pkt_buffer, &size) != PKT_OK) return; // encode pkt -> buf
                fprintf(stderr, "RESEND PAKET\n");
                print_sent_pkt(sendingWindow[i]);
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