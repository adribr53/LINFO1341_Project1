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
    if(received >= waited){ 
		 return received-waited<=32;
	}
    return 256+received-waited<=32;

}

int build_ack(pkt_t* pkt, uint32_t timestamp, uint8_t seqnum) {
    //fprintf(stderr, "ack sent of num %d\n", seqnum);
    pkt_set_type(pkt, PTYPE_ACK);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, MAX_WINDOW_SIZE);
    pkt_set_seqnum(pkt, seqnum);
    pkt_set_length(pkt, 0);
    pkt_set_timestamp(pkt, timestamp);
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
    pkt_t *pkt=pkt_new();
    while (1) {
        int isRunning=poll(&sfdPoll, 1 , 5000);
        if (!isRunning) {
            fprintf(stderr, "it'over\n");
            return;
        }
        if (sfdPoll.revents && POLLIN) {
            //fprintf(stderr, "still in it\n");
            nb=recv(sfd, data_buffer, MAX_PAYLOAD_SIZE+16, 0);
            //if (nb==0) return;
            // Decode pkt
            //fprintf(stderr, "Guillaume m'a écrit \n");
            if (pkt_decode(data_buffer, nb, pkt) == PKT_OK) {
                if (pkt_get_seqnum(pkt) == waitedSeqnum) {
                    //fprintf(stderr, "seqnum waited arrived : %d ", waitedSeqnum);
                    // parse window
                    int oldSeqnum=waitedSeqnum;
                    nb=write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
                    /*if (nb == 0) {
                        //fprintf(stderr, "Error while writing in outfile\n");
                        return;
                    }*/
                    uint32_t timestampAck=pkt_get_timestamp(pkt);
                    waitedSeqnum=(waitedSeqnum+1)%256;
                    receivePkt=1;
                    //fprintf(stderr, "next seqnum waited : %d\n", waitedSeqnum);
                    //if (peek(window)!=NULL) {
                        //fprintf(stderr, "what\n");
                        //fprintf(stderr, "seqnum of the head = %d\n", peek(window)->seqnum);
                    //}
                    // Unfill the window with consecutive seqnum (first pkt was not in the window)
                    while (!is_empty(window) && waitedSeqnum==peek(window)->seqnum) {
                        //fprintf(stderr, "Window parcourue\n");
                        //fprintf(stderr, "seqnum : %d\n", pkt_get_seqnum(peek(window)));
                        nb=write(outfd, peek(window)->payload, peek(window)->length);
                        /*if (nb==0) {
                            fprintf(stderr, "Error while writing in outfile\n"); 
                            return;
                        }*/
                        //fprintf(stderr, "about to be popped : %d\n", peek(window)->seqnum);
                        timestampAck=peek(window)->timestamp;
                        pop(window);
                        waitedSeqnum=(waitedSeqnum+1)%256;
                        receivePkt++;
                        //fprintf(stderr, "before the free\n");
                    }
                    reset(window, waitedSeqnum, oldSeqnum);
                    //fprintf(stderr, "\n");
                    //fprintf(stderr, "13\n");

                    // Send ack
                    pkt_t* ack_pkt = pkt_new();
                    build_ack(ack_pkt, timestampAck, waitedSeqnum);
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
                        // fprintf(stderr, "packet added to window\n");
                        //fprintf(stderr, "seqnum added : %d\n", pkt_get_seqnum(pkt));
                        add(window, pkt, waitedSeqnum);
                    } 
                    //fprintf(stderr, "new reçue pour ce seqnum HORS SEQ: %d\n", pkt_get_seqnum(pkt));
                    if (receivePkt != 0) {
                        // Resend last ack pkt
                        pkt_t* ack_pkt = pkt_new();
                        build_ack(ack_pkt, pkt_get_timestamp(pkt), waitedSeqnum);
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