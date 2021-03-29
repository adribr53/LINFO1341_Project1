// Code to compute the stats

#ifndef LINFO1341_PROJECT1_STAT_H
#define LINFO1341_PROJECT1_STAT_H
#include <stdlib.h>
#include <stdio.h>


typedef struct stat {
    int activated;
    FILE* fd;

    int data_sent;
    int data_received;
    int data_truncated_received;

    int ack_sent;
    int ack_received;
    int nack_sent;
    int nack_received;
    int packet_ignored;

    int is_sender;

    // if sender == 1
    int min_rtt;
    int max_rtt;
    int packet_retransmitted;

    // else
    int packet_duplicated;
} logger;

logger* new_logger(int is_sender, FILE* fd);
void remove_logger(logger* l);

void add_data_sent(logger* l);
void add_data_received(logger* l);
void add_data_truncated_received(logger* l);

void add_ack_sent(logger* l);
void add_ack_received(logger* l);
void add_nack_sent(logger* l);
void add_nack_received(logger* l);
void add_packet_ignored(logger* l);

// sender
void new_rtt(logger* l, int new_rtt); // does min & max rtt calc
void add_packet_retransmitted(logger* l);

// receiver
void add_packet_duplicated(logger* l);

#endif //LINFO1341_PROJECT1_STAT_H
