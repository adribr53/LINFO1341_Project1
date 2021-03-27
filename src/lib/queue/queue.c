//
// Created by felix on 10/03/21.
//

#include <stdlib.h>
#include "queue.h"
#include "../segment/packet_interface.h"

/*
 * @pre : -
 *
 * @post : the function returns a pointer to a new struct list that is empty
 *
 *
 */
queue *new_list() {
    queue *toR =(queue *) malloc(sizeof(queue));
    toR->size=0;
    toR->ptrAdd=0;
    toR->ptrPop=0;
    return toR;
}


void del_list(queue *list) {
    free(list);
}

/*
 * Add pkt to list
 * list : the list (prev init with new_list)
 * packet : ptr to the pkt to add (should not be null)
 */
int add(queue *list, pkt_t *packet) {
    if (packet==NULL) fprintf(stderr, "GOD PLEASE NOOO\n");
    list->window[list->ptrAdd]=packet;
    list->ptrAdd=(list->ptrAdd+1)%MAX_WINDOW_SIZE;
    list->size++;
    return 0;
}


/*
 * Return the first elem of the list without remove it
 * list : the list (prev init with new_list)
 */
pkt_t* peek(queue* list) {
    if (list->size!=0) {
        return list->window[list->ptrPop];
    }    
    return NULL;
}

/*
 * Remove the first elem of the list
 * list : the list (prev init with new_list)
 */
int dequeue(queue *list) {
    pkt_del(list->window[list->ptrPop]);
    list->window[list->ptrPop]=NULL;
    list->ptrPop=(1+list->ptrPop)%MAX_WINDOW_SIZE;
    list->size--;
    return 0;
}

/*
 * Return true if the list is empty
 * list : the list (prev init with new_list)
 */
int is_empty(queue *list) {
    return list->size==0;
}

/*
 * Return if the seqnum is greater or equal to the smallest element of the list
 * seqnum : seqnum to evaluate
 * list : the list (prev init with new_list)
 */
int is_higher_or_equal(int seqnum, queue *list) {
    // si l'ack est plus haut, on peut supprimer l'élément actuel. Pour savoir ça, on utilise is_higher() 
    if (is_empty(list)) return 0;
    pkt_t *ref=peek(list);
    uint8_t refSeqnum=pkt_get_seqnum(ref);
    if (seqnum<refSeqnum && refSeqnum-seqnum>100) { 
        return 1; // seqnum ---------------- refSeqnum
    }
    if (refSeqnum<seqnum && seqnum-refSeqnum>100) {
        return 0; // refSeqnum ---------------- seqnum
    }
    return seqnum>=refSeqnum;
}

/*
 * get the packet with a specific seqnum
 * list : the list (prev init with new_list)
 * receive_seqnum : seqnum to get
 * tmpPtk : on return will be pointed to the getted pkt
 */
int get(queue *list, uint8_t receive_seqnum, pkt_t **tmpPtk, int *supportIt)  {
    uint8_t index= (*supportIt==-1) ? list->ptrPop : *supportIt;  
    
    int firstTime=1;
    while (list->window[index]!=NULL && pkt_get_seqnum(list->window[index])!=receive_seqnum)
    {
        if (!firstTime && index==list->ptrPop) {
            return 0;
        }
        firstTime=0;
        index=(index+1)%MAX_WINDOW_SIZE;
    }
    if (list->window[index]==NULL) {
        return 0;
    }
    *tmpPtk=list->window[index];
    *supportIt=(index+1)%MAX_WINDOW_SIZE;
    return 1;
}

void print_seqnums(queue *list) {
    fprintf(stderr, "[ ");
    for (int i=0; i<MAX_WINDOW_SIZE; i++) {
        if (list->window[i]!=NULL) {
            fprintf(stderr, "%d ", pkt_get_seqnum(list->window[i]));
        }
        else {
            fprintf(stderr, "nul ");
        }
    }
    fprintf(stderr, " ]\n");
}