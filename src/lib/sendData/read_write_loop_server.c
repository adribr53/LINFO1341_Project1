#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>
#include "../sortedLinkedList/sortedLinkedList.h"

uint8_t curSeqnum=0;
list_t *window;

uint8_t seqnum_to_ack(uint8_t curSeqnum) {
    uint8_t toR=curSeqnum-1;
    if (toR<0) return toR+256;
    return toR;
}

void treatment_pkt(char *msg, unsigned int length, const int sfd, const int outfd) {
    pkt_t *pkt=pkt_new();
    if (pkt_decode(msg, length, pkt)!=PKT_OK || pkt_get_tr(pkt)==1) {
        fprintf(stderr, "packet corrompu\n");
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
        fprintf(stderr, "numero attendu reçu : %d\n", curSeqnum);
        // envoi du paquet recu et de ceux qui seraient dans le buffer
        add(window, pkt);
        int i=curSeqnum;
        pkt_t *nextPkt=peek(window);
        while (nextPkt!=NULL && pkt_get_seqnum(nextPkt)==i)
        {
            write(outfd, pkt_get_payload(nextPkt), pkt_get_length(nextPkt));
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
        fprintf(stderr, "numero inattendu reçu : %d, était attendu %d\n", pkt_get_seqnum(pkt), curSeqnum);
        int tmp=pkt_get_seqnum(pkt)-curSeqnum;
        if (tmp<0) tmp+=256;
        if (tmp>MAX_WINDOW_SIZE) return;
        add(window, pkt);
        // send ack
        pkt_t *pktAck=pkt_new();
        pkt_set_type(pktAck, PTYPE_ACK);
        pkt_set_tr(pktAck, 0);
        pkt_set_window(pktAck, MAX_WINDOW_SIZE);
        pkt_set_seqnum(pktAck, seqnum_to_ack(curSeqnum)); // i sert d'ack pour tous les paquets envoyés
        pkt_set_length(pktAck, 0);
        pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
        size_t nbBytes=10;
        char *reply=malloc(nbBytes);
        pkt_encode(pktAck, reply, &nbBytes);
        size_t wrote = send(sfd, reply, nbBytes, MSG_CONFIRM);
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
        // printf("=========LOOP===========\n");
        poll(&sfdPoll, 1 , -1);
        if (sfdPoll.revents & POLLIN) {
            //read(socket) -> write(stdout) ou send(socket)
            nb=recv(sfd, buf, MAX_PAYLOAD_SIZE+16, 0);
            if (nb==0) return;
            treatment_pkt(buf, nb, sfd, outfd);
        }
    }

}
