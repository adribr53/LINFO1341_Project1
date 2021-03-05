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
    printf("=== PAKET SENT ===\n");
    printf("type => %d\n", pkt_get_type(paket));
    printf("TR => %d\n", pkt_get_tr(paket));
    printf("WINDOW => %d\n", pkt_get_window(paket));
    printf("Length => %d\n", pkt_get_length(paket));
    printf("Seqnum => %d\n", pkt_get_seqnum(paket));
    printf("timestamp => %d\n", pkt_get_timestamp(paket));
    printf("crc1 => %d\n", pkt_get_crc1(paket));
    printf("Payload => %s\n", pkt_get_payload(paket));
    printf("crc2 => %d\n", pkt_get_crc2(paket));
    printf("=== BRRRRRRR ===\n\n");
}

int is_in_window_interval(uint8_t seqnum, uint8_t start, uint8_t end) {
    if (seqnum > 31) printf("WTF ???\n");
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
    uint32_t TIMEOUT = 276447232;
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
        poll(pfd, 2 , TIMEOUT);
        if (pfd[1].revents & POLLIN) {
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
                if (err<0) return;

                endWindow = (endWindow+1)%31;
                pktInWindow++;
            }
            // waitForAck = TRUE;
        }
        if (pfd[0].revents & POLLIN) {
            // read(socket) -> write(stdout)
            nb=read(sfd, pkt_buffer, 12);
            socketResp = pkt_decode(pkt_buffer, 12, socketPkt);
            if (socketResp == PKT_OK) { // if pkt not ok we just drop it
                switch ((int) pkt_get_type(socketPkt)) {
                    case 2:
                        // ACK
                        receive_seqnum = pkt_get_seqnum(socketPkt);
                        if (is_in_window_interval(receive_seqnum, startWindow, endWindow)) { // pkt stored in receiver buffer or duplicate
                            int i = startWindow;
                            while (receive_seqnum != pkt_get_seqnum(sendingWindow[i % 31]) && i < 31) i++;
                            if (i == 31) return; // should never happen
                            if (pkt_get_timestamp(sendingWindow[i % 31]) == pkt_get_timestamp(socketPkt)) { // avoid duplicate packets
                                i++;
                                startWindow = (startWindow+i)%31;
                                pktInWindow -= i;
                                sendingWindowSize = sendingWindowSize == 31 ? 31 : sendingWindowSize+1;
                            }
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
            if (pkt_get_timestamp(sendingWindow[i]) + TIMEOUT < (uint32_t) clock()) {
                // RT expire => resend the pkt
                pkt_set_timestamp(sendingWindow[i], (uint32_t) clock()); // update timestamp
                size_t size = (size_t) MAX_PAYLOAD_SIZE + 16;
                if (pkt_encode(sendingWindow[i], pkt_buffer, &size) != PKT_OK) return; // encode pkt -> buf
                print_sent_pkt(sendingWindow[i]);
                err=write(sfd, pkt_buffer, size); // sendto ???
                if (err<0) {
                    fprintf(stderr, "Value of errno: %d\n", errno);
                    fprintf(stderr, "Error opening file: %s\n", strerror( errno ));
                    printf("je suis con\n");
                    return;
                }
            }
            i = (i + 1) % 31;
        }
    }
}
