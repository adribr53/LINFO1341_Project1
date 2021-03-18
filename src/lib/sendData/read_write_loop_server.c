#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>
#include "../sortedLinkedList/sortedLinkedList.h"

unsigned long nbPkWrt=0;
uint8_t curSeqnum=0;
list_t *window;
uint8_t seqnum_to_ack(uint8_t curSeqnum) {
    uint8_t toR=curSeqnum-1;
    if (toR<0) return toR+256;
    return toR;
}

int cmp_seqnum(uint8_t s1, uint8_t s2) {
    if (s1<s2 && s2-s1>100) return 1; // s1.................s2
    if (s1>=s2 && s1-s2>100) return 0; // s2................s1
    return s1>s2; // s2...s1 ou s1....s2
}

void treatment_pkt(char *msg, unsigned int length, const int sfd, const int outfd) {
    pkt_t *pkt=pkt_new();
    if (pkt_decode(msg, length, pkt)!=PKT_OK || pkt_get_tr(pkt)==1) {
        pkt_t *pktAck=pkt_new();
        pkt_set_type(pktAck, PTYPE_NACK);
        pkt_set_tr(pktAck, 0);
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        pkt_set_seqnum(pktAck, seqnum_to_ack(curSeqnum)); // i sert d'ack pour tous les paquets envoyés
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(pktAck, reply, &nbBytes);
        size_t wrote = send(sfd, reply, nbBytes, MSG_CONFIRM);
        pkt_del(pkt);
        return;
    } else if (pkt_get_seqnum(pkt)==curSeqnum) {
        // envoi du paquet recu et de ceux qui seraient dans le buffer
        write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
        nbPkWrt++;
        int i=(curSeqnum+1)%256;
        pkt_t *nextPkt=peek(window);
        if (nextPkt!=NULL && pkt_get_seqnum(nextPkt)==curSeqnum) {
            pop(window); // si le num attendu etait dans la window, retrait
        }
        nextPkt=peek(window);
        while (nextPkt!=NULL && pkt_get_seqnum(nextPkt)==i)
        {
            write(outfd, pkt_get_payload(nextPkt), pkt_get_length(nextPkt));
            nbPkWrt++;
            pop(window);
            nextPkt=peek(window);
            i=(i+1)%256;
        }
        // envoi de l'ack
        pkt_t *pktAck=pkt_new();
        pkt_set_type(pktAck, PTYPE_ACK);
        pkt_set_tr(pktAck, 0);
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        curSeqnum=i;
        pkt_set_seqnum(pktAck, seqnum_to_ack(curSeqnum)); // i sert d'ack pour tous les paquets envoyés
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(pktAck, reply, &nbBytes);
        size_t wrote = send(sfd, reply, nbBytes, MSG_CONFIRM);
        pkt_del(pkt);
        free(reply);
    } else {
        if (cmp_seqnum(curSeqnum, pkt_get_seqnum(pkt))==0) {
            add(window, pkt);
        }
        // send ack
        pkt_t *pktAck=pkt_new();
        pkt_set_type(pktAck, PTYPE_ACK);
        pkt_set_tr(pktAck, 0);
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        pkt_set_seqnum(pktAck, seqnum_to_ack(curSeqnum));
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(pktAck, reply, &nbBytes);
        size_t wrote = send(sfd, reply, nbBytes, MSG_CONFIRM);
        /*if (nbOutSeq==100) {
            for (int i=0; i<5; i++) {
                size_t wrote = send(sfd, reply, nbBytes, MSG_CONFIRM);
            }
            nbOutSeq=0;
        }*/
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
            nb=recv(sfd, buf, MAX_PAYLOAD_SIZE+16, 0);
            if (nb==0) return;
            treatment_pkt(buf, nb, sfd, outfd);
        }
    }
}
