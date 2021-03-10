//
// Created by felix on 10/03/21.
//

#ifndef LINFO1341_PROJECT1_LINKEDLIST_H
#define LINFO1341_PROJECT1_LINKEDLIST_H
#include "../segment/packet_interface.h"
#include <stdlib.h>

typedef struct _node {
    struct _node *next;
    pkt_t *pkt;
} node;


typedef struct list {
    struct _node *first;
    struct _node *last;
} linkedList;

linkedList *new_list();
node *new_node(pkt_t *packet);
int add(linkedList *list, pkt_t *packet);
pkt_t* peek(linkedList* list);
pkt_t* pop(linkedList *list);
int is_empty(linkedList *list);
int get(linkedList *list, int seqnum, pkt_t** resp);
void print_list(linkedList* list);
void freeNode(node* n);
void freeList(linkedList * list);

#endif //LINFO1341_PROJECT1_LINKEDLIST_H
