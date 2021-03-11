//
// Created by felix on 10/03/21.
//

#include <stdlib.h>
#include "linkedlist.h"
#include "../segment/packet_interface.h"

/*
 * @pre : -
 *
 * @post : the function returns a pointer to a new struct list that is empty
 *
 *
 */
linkedList *new_list() {
    linkedList *toR =(linkedList *) malloc(sizeof(linkedList));
    return toR;
}

node* new_node(pkt_t* pkt) {
    node* resp = (node *) malloc(sizeof(node));
    if (resp != NULL) resp->pkt=pkt;
    return resp;
}

int add(linkedList *list, pkt_t *packet) {
    if (packet == NULL) fprintf(stderr, "GOD PLEASE NOOO\n");
    node* newNode = new_node(packet);
    if (newNode == NULL) return -1;
    if (list->last == NULL) {
        list->first=newNode;
        list->last=newNode;
    } else {
        list->last->next=newNode;
        list->last=newNode;
    }
    return 0;
}

pkt_t* peek(linkedList* list) {
    if (list->first != NULL) return list->first->pkt;
    return NULL;
}

pkt_t* pop(linkedList *list) {
    pkt_t* resp = peek(list);
    if (resp != NULL) {
        node* toDel = list->first;
        list->first=list->first->next;
        if (pkt_get_seqnum(toDel->pkt) == pkt_get_seqnum(list->last->pkt)) {
            list->last = NULL;
        }
        // free(toDel);
    }
    return resp;
}

int is_empty(linkedList *list) {
    return list->first == NULL;
}

int get(linkedList *list, int seqnum, pkt_t** resp) {
    if (is_empty(list)) return -1;
    node* curr = list->first;
    while (curr != NULL) {
        if (pkt_get_seqnum(curr->pkt) == seqnum) {
            *resp = curr->pkt;
            return 0;
        }
        curr=curr->next;
    }
    return -2;
}

void print_list(linkedList* list) {
    fprintf(stderr, "[");
    node* curr = list->first;
    node* last = list->last;
    while (curr != NULL) {
        fprintf(stderr, " %d", pkt_get_seqnum(curr->pkt));
        curr=curr->next;
    }
    fprintf(stderr, " ]");
    if (last == NULL) fprintf(stderr, "\tLast == NULL\n");
    else fprintf(stderr, "\t last : %d\n", pkt_get_seqnum(last->pkt));
}

void freeNode(node* n) {
    free(n);
}

void freeList(linkedList * list) {
    free(list);
}