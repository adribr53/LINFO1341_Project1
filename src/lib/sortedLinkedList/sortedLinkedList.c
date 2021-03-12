#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "../segment/packet_interface.h"
#include "sortedLinkedList.h"
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
int cmp(pkt_t *p1, pkt_t *p2) { // p1 > p2 => true
    uint8_t s1=pkt_get_seqnum(p1);
    uint8_t s2=pkt_get_seqnum(p2);
    if (s1<s2 && s2-s1>100) return 1; // s1.................s2
    if (s1>=s2 && s1-s2>100) return 0; // s2................s1
    return s1>s2; // s2...s1 ou s1....s2
}

int pkt_equal(pkt_t* p1, pkt_t* p2) {
    return pkt_get_seqnum(p1) == pkt_get_seqnum(p2);
}


void printList(list_t *list) {
    node_t *cur=list->first_t;
    int i=0;
    while (cur!=NULL) {
        fprintf(stderr, "num de séq de l'élément current %d\n", pkt_get_seqnum(cur->packet_t));
        cur=cur->next_t;
        i++;
    }
    fprintf(stderr, "taille : %d\n", i);
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
int add(list_t *list, pkt_t *packet) {
    node_t *newNode = malloc(sizeof(node_t));
    newNode->packet_t = packet;
    if (list->first_t == NULL) {
        list->first_t = newNode;
    } else {
        node_t *curr = list->first_t;
        if (cmp(curr->packet_t , packet)) { // first après packet -> packet devient first
            newNode->next_t=curr;
            list->first_t = newNode;
        } else {
            while (curr->next_t != NULL && cmp(packet, curr->next_t->packet_t)) {
                curr = curr->next_t;
            }
            if (pkt_equal(packet, curr->packet_t)) return -1;
            newNode->next_t = curr->next_t;
            curr->next_t = newNode;
        }
    }/*
    fprintf(stderr, "add finished\n");
<<<<<<< HEAD
=======
    printList(list);*/
>>>>>>> 480e46ee03e650173bf25a374ea7720b9c58b72f
    return 0;
}


/*
 * @pre : list is a (non null) pointer to a list
 *
 * @post : return the first elem
 */
pkt_t* peek(list_t* list) {
    if (list->first_t==NULL) return NULL;
    return list->first_t->packet_t;
}

/*
 * @pre : list is a (non null) pointer to a list
 *        
 * @post : return the first elem and delete it if it exist
 */
pkt_t* pop(list_t *list) {
    pkt_t* resp = peek(list);
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
    return list->first_t == NULL;
}



/*int main() {
    list_t *list=new_list();

    pkt_t *test0=pkt_new();
    pkt_set_seqnum(test0, 0);
    add(list, test0);

    pkt_t *test5=pkt_new();
    pkt_set_seqnum(test5, 5);
    add(list, test5);

    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3, 3);
    add(list, test3);

    pkt_t *test6=pkt_new();
    pkt_set_seqnum(test6, 6);
    add(list, test6);

    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4, 4);
    add(list, test4);

    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1, 1);
    add(list, test1);

    while (peek(list)!=NULL) {
        printf("%d\n", pkt_get_seqnum(pop(list)));
    }



}*/