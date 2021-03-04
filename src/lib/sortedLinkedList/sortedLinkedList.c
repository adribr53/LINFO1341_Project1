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
    if (toR == NULL) return NULL;
    toR->first = NULL;
    return toR;
}

/*
 * @pre : value is an unsigned long long
 *        
 * @post : the function returns a pointer to a new Node,
 *         			 that contains value as value
 *        
 */
node_t *new_node(pkt_t *packet) {
    node_t *toR= (node_t *) malloc(sizeof(node_t));
    if (toR==NULL) return NULL;
    toR->next_t=NULL;
    toR->packet_t=packet;
    return toR;
}

/* cmp de seqnum */
int cmp(pkt *p1, pkt *p2) {
    int s1=pkt_get_seqnum(p1);
    int s2=pkt_get_seqnum(p2);
    if (s1<s2 && s2-s1>100) return 1; // s1.................s2
    if (s1>=s2 && s1-s2>100) return 0; // s2................s1
    return s1>s2; // s2...s1 ou s1....s2
}

/*
 * @pre : list is a pointer to a struct list
 *        value is an unsigned long long
 *        mallocrelated to newNode won't fail
 * @post : the function returns 0
 *         the number value is added to the struct pointed by list
 *         the size is increased
 * Rem :   (if malloc fails, return -1 and does nothing else)
 */
int add(list_t *list, pkt_t packet) {
    
    node_t *toAdd = new_node(packet);
    if (toAdd == NULL || list == NULL) return 1;
    node_t *cur=list->first_t;
    if (cur==NULL) {
        list->first_t=toAdd;
    } else {
        do {
            if ()
        } while ();
    }

    return 0;
}


/*
 * @pre : list is a pointer to a list
 *        
 * @post : the function removes the last element from the pointed list 
 *              return -1 if the list is empty
 *         		returns the removed value
 *        		reduces the size
 */
int8_t remove(list_t *list, uint8_t seqnum) {
    if (list->size == 0) {return -1;}
    uint8_t toR = list->first->value;
    node_t *toFree = list->first;
    if (list->size == 1) {
        list->first = NULL;
        list->last = NULL;
    } else {
        list->first = list->first->next;
    }
    free(toFree);
    list->size--;
    return toR;
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



