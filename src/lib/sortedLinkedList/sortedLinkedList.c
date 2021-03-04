#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "../segment/packet_interface.h"
#include "sortedLinkedList.h."
/*
 * @pre : -
 *        
 * @post : the function returns a pointer to a new struct list that is empty
 *         		
 *        
 */
list_t *new_list() {
    list_t *toR =(list_t *) malloc(sizeof(list_t));
    return toR;
}

/*
 * @pre : value is an unsigned long long
 *        
 * @post : the function returns a pointer to a new Node,
 *         			 that contains value as value
 *        
 */
/*node_t *new_node(pkt_t *packet) {
    node_t *toR= (node_t *) malloc(sizeof(node_t));
    if (toR==NULL) return NULL;
    toR->next_t=NULL;
    toR->packet_t=packet;
    return toR;
}*/

/* cmp de seqnum */
int cmp(pkt *p1, pkt *p2) { // p1 > p2 => true
    int s1=pkt_get_seqnum(p1);
    int s2=pkt_get_seqnum(p2);
    if (s1<s2 && s2-s1>100) return 1; // s1.................s2
    if (s1>=s2 && s2-s1>100) return 0; // s2................s1
    return s1>s2; // s2...s1 ou s1....s2
}

int pkt_equal(pkt* p1, pkt* p2) {
    return pkt_get_seqnum(p1) == pkt_get_seqnum(p2);
}
/*
 * @pre : list is a pointer to a struct list
 *        value is an unsigned long long
 *        malloc related to newNode won't fail
 * @post : the function returns 0
 *         the number value is added to the struct pointed by list
 *         the size is increased
 * Rem :   (if malloc fails, return -1 and does nothing else)
 */
int add(list_t *list, pkt_t packet) {
    node_t *newNode = malloc(sizeof(node_t));
    newNode->pkt_t = &packet;
    if (list->first_t == NULL) {
        list->first_t = newNode;
    } else {
        node_t *curr = list->first_t;
        if (cmp(curr->packet_t , packet)) {
            newNode->next_t=curr;
            list->first_t = newNode;
        } else {
            while (curr->next_t != NULL && cmp(packet, curr->next_t->packet_t)) {
                curr = curr->next_t;
            }
            if (pkt_equal(&packet, curr->packet_t)) return -1;
            newNode->next_t = curr->next_t;
            curr->next_t = newNode;
        }
    }
    return 0;
}


/*
 * @pre : list is a (non null) pointer to a list
 *
 * @post : return the first elem
 */
pkt* peek(list_t* list) {
    return list->first_t;
}

/*
 * @pre : list is a (non null) pointer to a list
 *        
 * @post : return the first elem and delete it if it exist
 */
struct pkt* pop(list_t *list, ) {
    pkt* resp = peek(list);
    if (resp != NULL)
        list->first_t = list->first_t->next_t;
    return resp;
}


/*
 * @pre : list is a pointer to a struct list
 *        
 * @post : the function returns 1 if the list is empty 
 *         			0 otherwise
 *        
 */
int is_empty(list_t *list) {
    return list->first == NULL;
}



