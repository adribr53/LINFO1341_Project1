//
// Created by felix on 10/03/21.
//

#ifndef LINFO1341_PROJECT1_LINKEDLIST_H
#define LINFO1341_PROJECT1_LINKEDLIST_H
#include "../segment/packet_interface.h"
#include <stdlib.h>

typedef struct list {
    pkt_t *window[MAX_WINDOW_SIZE];
    int size;
    uint8_t ptrAdd;
    uint8_t ptrPop;
} linkedList;

linkedList *new_list();
void del_list(linkedList *list);
int add(linkedList *list, pkt_t *packet);
pkt_t *peek(linkedList* list);
int pop(linkedList *list);
int is_empty(linkedList *list);
int is_higher_or_equal(int seqnum, linkedList *list);
int get(linkedList *list, uint8_t seqnum, pkt_t** resp, int *supportIt);
void print_seqnums(linkedList *list);

#endif //LINFO1341_PROJECT1_LINKEDLIST_H
