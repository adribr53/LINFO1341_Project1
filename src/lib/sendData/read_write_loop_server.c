#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>

uint8_t curSeqnum=0;
pkt_t *window[32]; // remplacer par structure qu'on trie en fonction du seqnum


void treatment_pkt(char *msg, unsigned int length, const int sfd, const int outfd) {
    pkt_t *pkt=pkt_new();
    if (pkt_decode(msg, length, pkt)!=PKT_OK) {
        fprintf(stderr, "packet ")
        return;
    }
    if (pkt_get_type(pkt)==PTYPE_DATA) {
        if (pkt_get_seqnum(pkt)==curSeqnum) {
            // envoi du paquet recu et de ceux qui seraient dans le buffer
            if (pkt_get_tr(pkt)==0) {
                write(outfd, pkt_get_payload(window[curSeqnum]), pkt_get_length(window[curSeqnum]));
            }
            window[curSeqnum]=NULL;
            int i=(curSeqnum+1)%256;
            for (i; window[i]!=NULL; i=(i+1)%256) {
                write(outfd, pkt_get_payload(window[i]), pkt_get_length(window[i]));
                window[i]=NULL;
            }
            // envoi de l'ack
            pkt_t pktAck=pkt_new();
            pkt_set_type(pktAck, PTYPE_ACK);
            pkt_set_tr(pktAck, 0);
            pkt_set_window(pktAck, MAX_WINDOW_SIZE);
            pkt_set_seqnum(i); // i sert d'ack pour tous les paquets envoyés
            pkt_set_length(pktAck, 0);
            pkt_set_timestamp(pktAck, pkt_get_timestamp(pkt));
            pkt_set_crc1(pkt, pkt_comp_crc1(pkt, msg));
            curSeqnum=(i+1)%256;
            size_t nbBytes=10;
            char *reply=malloc(nbBytes);
            pkt_encode(packet, ackBuffer, &nbBytes);
            size_t wrote = send(sfd, ackBuffer, nbBytes, MSG_CONFIRM);
        } else {
            int tmp=pkt_get_seqnum()-curSeqnum;
            if (tmp<0) tmp+=256;
            if (tmp>MAX_WINDOW_SIZE) return;

            // TODO mettre le paquet dans la structure, en maintenant le tri


        }
    }

}

void read_write_loop_server(const int sfd, const int outfd) {

    struct pollfd sfdPoll;
    sfdPoll.fd=sfd;
    sfdPoll.events=POLLIN;

    char buf[MAX_PAYLOAD_SIZE+16];
    int nb;
    int err;
    while (1) {
        poll(&sfdPoll, 1 , -1);
        if (sfdPoll.revents & POLLIN) {
            //read(socket) -> write(stdout) ou send(socket)
            nb=read(sfd, buf, MAX_PAYLOAD_SIZE+16);
            if (nb==0) return;
            treatment_pkt(buf, nb, sfd, outfd);





            //err=write(outfd, buf, nb);
            //if (err<0) return;
        }
    }

}
