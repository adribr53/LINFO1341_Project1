//
// Created by felix on 10/03/21.
//

#ifndef LINFO1341_PROJECT1_QUEUE_H
#define LINFO1341_PROJECT1_QUEUE_H
#include "../segment/packet_interface.h"
#include <stdlib.h>

typedef struct list {
    pkt_t *window[MAX_WINDOW_SIZE];
    int size;
    uint8_t ptrAdd;
    uint8_t ptrPop;
} queue;

queue *new_list();
void del_list(queue *list);
int add(queue *list, pkt_t *packet);
pkt_t *peek(queue* list);
int dequeue(queue *list);
int is_empty(queue *list);
int is_higher_or_equal(int seqnum, queue *list);
int get(queue *list, uint8_t seqnum, pkt_t** resp, int *supportIt);
void print_seqnums(queue *list);

#endif //LINFO1341_PROJECT1_QUEUE_H
