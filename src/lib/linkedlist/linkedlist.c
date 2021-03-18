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
    toR->size=0;
    toR->ptrAdd=0;
    toR->ptrPop=0;
    return toR;
}


void del_list(linkedList *list) {
    free(list);
}

/*
 * Add pkt to list
 * list : the list (prev init with new_list)
 * packet : ptr to the pkt to add (should not be null)
 */
int add(linkedList *list, pkt_t *packet) {
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
pkt_t* peek(linkedList* list) {
    if (list->size!=0) {
        return list->window[list->ptrPop];
    }    
    return NULL;
}

/*
 * Remove the first elem of the list
 * list : the list (prev init with new_list)
 */
int pop(linkedList *list) {
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
int is_empty(linkedList *list) {
    return list->size==0;
}

/*
 * Return if the seqnum is greater or equal to the smallest element of the list
 * seqnum : seqnum to evaluate
 * list : the list (prev init with new_list)
 */
int is_higher_or_equal(int seqnum, linkedList *list) {
    // si l'ack est plus haut, on peut supprimer l'élément actuel. Pour savoir ça, on utilise is_higher() 
    if (is_empty(list)) return 0;
    pkt_t *ref=peek(list);
    if (ref==NULL) printf("whut\n");
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
int get(linkedList *list, uint8_t receive_seqnum, pkt_t **tmpPtk, int *supportIt)  {
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

void print_seqnums(linkedList *list) {
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

/*int main() {
    linkedList *list=new_list();

    pkt_t *test0=pkt_new();
    pkt_set_seqnum(test0, 0);
    pkt_set_payload(test0, "br0", 3);
    add(list, test0);
    print_seqnums(list);

    pkt_t *test5=pkt_new();
    pkt_set_seqnum(test5, 1);
    pkt_set_payload(test5, "br1\n", 3);
    add(list, test5);
    print_seqnums(list);
    

    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3, 2);
    pkt_set_payload(test3, "br2\n", 3); 
    add(list, test3);
    print_seqnums(list);


    pkt_t *test6=pkt_new();
    pkt_set_seqnum(test6, 3);
    pkt_set_payload(test6, "br3\n", 3);
    add(list, test6);
    print_seqnums(list);


    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4, 4);
    pkt_set_payload(test4, "br4\n", 3);
    add(list, test4);
    print_seqnums(list);


    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1, 5);
    pkt_set_payload(test1, "br5\n", 3);
    add(list, test1);
    print_seqnums(list);
    printf("second part\n");
    while (is_higher_or_equal(3, list))
    {
        printf("this is our chance\n");
        pop(list);
        
    }
    print_seqnums(list);

    pkt_t *testbr=pkt_new();
    pkt_set_seqnum(test0, 6);
    pkt_set_payload(test0, "br0", 6);
    add(list, test0);
    print_seqnums(list);

    pkt_t *testbrr=pkt_new();
    pkt_set_seqnum(test5, 7);
    pkt_set_payload(test5, "br1\n", 7);
    add(list, test5);
    print_seqnums(list);
    printf("Let's do it, now !\n");

    int *supportInt=malloc(sizeof(int));
    *supportInt=-1; // outil pour itérer
    pkt_t *tmpPtk = peek(list);
    uint8_t receive_seqnum = pkt_get_seqnum(tmpPtk);
    *supportInt=-1; // outil pour itérer
    while (get(list, receive_seqnum, &tmpPtk, supportInt)) {
            // Set new timestamp
        pkt_set_timestamp(tmpPtk, (uint32_t) 3);
        printf("%d and %d\n", pkt_get_seqnum(tmpPtk), pkt_get_timestamp(tmpPtk));
            // resend    
        receive_seqnum=(receive_seqnum+1)%MAX_WINDOW_SIZE;
    }

    print_seqnums(list);
    


    free(list);


}*/
