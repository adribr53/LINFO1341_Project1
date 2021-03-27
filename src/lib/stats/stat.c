#include "stat.h"

logger* new_logger(int is_sender, FILE* fd) {
    logger* resp = malloc(sizeof(logger));

    int activated = 0;
    if (fd != NULL) activated = 1;

    resp->activated = activated;
    if (activated == 1) {
        resp->fd = fd;

        resp->data_sent = 0;
        resp->data_received = 0;
        resp->data_truncated_received = 0;

        resp->ack_sent = 0;
        resp->ack_received = 0;
        resp->nack_sent = 0;
        resp->nack_received = 0;
        resp->packet_ignored = 0;

        resp->is_sender = is_sender;
        if (is_sender == 1) {
            resp->min_rtt = 2147483647; // Set as max int
            resp->max_rtt = 0;
            resp->packet_retransmitted = 0;
        } else {
            resp->packet_duplicated = 0;
        }
    }
    return resp;
}

int write_in_file(logger* l) {
    fprintf(l->fd, "data_sent:%d\n", l->data_sent);
    fprintf(l->fd, "data_received:%d\n", l->data_received);
    fprintf(l->fd, "data_truncated_received:%d\n", l->data_truncated_received);

    fprintf(l->fd, "ack_sent:%d\n", l->ack_sent);
    fprintf(l->fd, "ack_received:%d\n", l->ack_received);
    fprintf(l->fd, "nack_sent:%d\n", l->nack_sent);
    fprintf(l->fd, "nack_received:%d\n", l->nack_received);
    fprintf(l->fd, "packet_ignored:%d\n", l->packet_ignored);

    if (l->is_sender == 1) {
        fprintf(l->fd, "min_rtt:%d\n", l->min_rtt);
        fprintf(l->fd, "max_rtt:%d\n", l->max_rtt);
        fprintf(l->fd, "packet_retransmitted:%d\n", l->packet_retransmitted);
    } else {
        fprintf(l->fd, "packet_duplicated:%d\n", l->packet_duplicated);
    }
    return 0;
}

void remove_logger(logger* l) {
    if (l->activated == 1) write_in_file(l);
    free(l);
}

void add_data_sent(logger* l) {
    if (l->activated == 1) l->data_sent++;
}
void add_data_received(logger* l) {
    if (l->activated == 1) l->data_received++;
}
void add_data_truncated_received(logger* l) {
    if (l->activated == 1) l->data_truncated_received++;
}

void add_ack_sent(logger* l) {
    if (l->activated == 1) l->ack_sent++;
}
void add_ack_received(logger* l) {
    if (l->activated == 1) l->ack_received++;
}
void add_nack_sent(logger* l) {
    if (l->activated == 1) l->nack_sent++;
}
void add_nack_received(logger* l) {
    if (l->activated == 1) l->nack_received++;
}
void add_packet_ignored(logger* l) {
    if (l->activated == 1) l->packet_ignored++;
}

// sender
void new_rtt(logger* l, int new_rtt) {
    if (l->activated == 1) if (l->is_sender == 1) {
        if (new_rtt < l->min_rtt) l->min_rtt = new_rtt;
        if (l->max_rtt < new_rtt) l->max_rtt = new_rtt;
    }
}
void add_packet_retransmitted(logger* l) {
    if (l->activated == 1) if (l->is_sender == 1) l->packet_retransmitted++;
}

// receiver
void add_packet_duplicated(logger* l) {
    if (l->activated == 1) if (l->is_sender == 0) l->packet_duplicated++;
}