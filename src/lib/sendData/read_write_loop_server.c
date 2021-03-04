#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>
#include "../sortedLinkedList/sortedLinkedList.h"

uint8_t curSeqnum=0;
list_t window;

void treatment_pkt(char *msg, unsigned int length, const int sfd, const int outfd) {
    pkt_t *pkt=pkt_new();
    if (pkt_decode(msg, length, pkt)!=PKT_OK) {
        fprintf(stderr, "packet corrupted")
        return;
    }
    if (pkt_get_tr(pkt)==1) {
        // envoi de l'ack
        pkt_t pktAck=pkt_new();
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        pkt_set_seqnum(curSeqnum); // i sert d'ack pour tous les paquets envoyés
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        pkt_set_crc1(pkt, pkt_comp_crc1(pkt, msg));
        if (i<0) curSeqnum+=256;
        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(packet, ackBuffer, &nbBytes);
        size_t wrote = send(sfd, ackBuffer, nbBytes, MSG_CONFIRM);
        pkt_del(pkt);
    } else if (pkt_get_seqnum(pkt)==curSeqnum) {
        // envoi du paquet recu et de ceux qui seraient dans le buffer
        write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
        pkt=peek(window);
        if (pkt!=NULL && pkt_get_seqnum(pkt)==curSeqnum) {
            pop(window);
        }

        do {
            int i=(curSeqnum+1)%256;
            pkt=peek(window);
            if (pkt!=NULL && pkt_get_seqnum(pkt)==i) write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
            else break;
        } while (1)

        // envoi de l'ack
        pkt_t pktAck=pkt_new();
        pkt_set_tr(pktAck, 0);
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        curSeqnum=i-1;
        if (i<0) curSeqnum+=256;
        pkt_set_seqnum(curSeqnum); // i sert d'ack pour tous les paquets envoyés
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        pkt_set_crc1(pkt, pkt_comp_crc1(pkt, msg));

        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(packet, ackBuffer, &nbBytes);
        size_t wrote = send(sfd, ackBuffer, nbBytes, MSG_CONFIRM);
        pkt_del(pkt);
    } else {
        int tmp=pkt_get_seqnum()-curSeqnum;
        if (tmp<0) tmp+=256;
        if (tmp>MAX_WINDOW_SIZE) return;
        add(window, pkt);
    }
}



void read_write_loop_server(const int sfd, const int outfd) {

    struct pollfd sfdPoll;
    sfdPoll.fd=sfd;
    sfdPoll.events=POLLIN;

    window=new_list();
    char buf[MAX_PAYLOAD_SIZE+16];
    int nb;
    while (1) {
        poll(&sfdPoll, 1 , -1);
        if (sfdPoll.revents & POLLIN) {
            //read(socket) -> write(stdout) ou send(socket)
            nb=read(sfd, buf, MAX_PAYLOAD_SIZE+16);
            if (nb==0) return;
            treatment_pkt(buf, nb, sfd, outfd);
        }
    }

}
