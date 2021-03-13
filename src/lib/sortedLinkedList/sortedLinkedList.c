#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "../segment/packet_interface.h"
#include "sortedLinkedList.h"
#include "string.h"

int ind(uint8_t toPlace, uint8_t waited) {
    int diff=toPlace-waited;
    if (diff<0) diff+=256;
    return diff; 
}

/*
 * @pre : -
 *        
 * @post : the function returns a pointer to a new struct list that is empty
 *         		
 *        
 */
list_t *new_list() {
    list_t *toR =(list_t *) malloc(sizeof(list_t));
    toR->size=0;
    toR->ptr=0;
    return toR;
}


/*
 * @pre : list is a pointer to a struct list
 *        waited is the waited seqnum
 *        
 * @post : the function returns 0
 *         the value is added to the struct pointed by list
 * Rem :   (if malloc fails, return -1 and does nothing else)
 */
int add(list_t *list, pkt_t *packet, uint8_t waited) {
    int index=ind(pkt_get_seqnum(packet), waited);
    if (list->window[index]!=NULL) {
        return -1;
    }
    pst_t *toAdd=malloc(sizeof(pst_t));
    if (toAdd==NULL) {
        fprintf(stderr, "Failed to add");
    }
    toAdd->length=pkt_get_length(packet);
    memcpy(toAdd->payload, pkt_get_payload(packet), toAdd->length);
    toAdd->seqnum=pkt_get_seqnum(packet);
    toAdd->timestamp=pkt_get_timestamp(packet);
    list->window[index]=toAdd;
    list->size++;
    return 0;
}


/*
 * @pre : list is a (non null) pointer to a list
 *
 * @post : return the first elem
 */
pst_t *peek(list_t* list) {
    if (list->size==0) {
        return NULL;
    }
    while (list->window[list->ptr]==NULL)
    {
        list->ptr++;
    }
    return list->window[list->ptr];
}

int pop(list_t *list) {
    free(list->window[list->ptr]);
    list->window[list->ptr]=NULL;
    list->size--;
    return 0;
}

void reset(list_t *list, uint8_t newWaited, uint8_t oldWaited) {
    int shift=ind(newWaited, oldWaited);
    int nb=0;
    for (int i=shift; (i<MAX_WINDOW_SIZE) && (nb<list->size); i++) {
        if (list->window[i]!=NULL) {
            list->window[i-shift]=list->window[i];
            list->window[i]=NULL;
            nb++;
        }
    }
    list->ptr=0;
}

int is_empty(list_t *list) {
    return list->size==0;
}

/*int main() {
    list_t *list=new_list();

    pkt_t *test0=pkt_new();
    pkt_set_seqnum(test0, 0);
    pkt_set_payload(test0, "br0", 3);
    add(list, test0, 0);
    pkt_del(test0);

    pkt_t *test5=pkt_new();
    pkt_set_seqnum(test5, 5);
    pkt_set_payload(test5, "br5\n", 3);
    add(list, test5, 0);
    pkt_del(test5);
    

    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3, 3);
    pkt_set_payload(test3, "br3\n", 3); 
    add(list, test3, 0);
    pkt_del(test3);


    pkt_t *test6=pkt_new();
    pkt_set_seqnum(test6, 6);
    pkt_set_payload(test6, "br6\n", 3);
    add(list, test6, 0);
    pkt_del(test6);


    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4, 4);
    pkt_set_payload(test4, "br4\n", 3);
    add(list, test4, 0);
    pkt_del(test4);


    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1, 0);
    pkt_set_payload(test1, "br0\n", 3);
    add(list, test1, 0);
    pkt_del(test1);


    while (peek(list)!=NULL) {
        printf("test\n");
        printf("%d\n", peek(list)->seqnum);
        printf("%s\n", peek(list)->payload);
        pop(list);
        printf("still in it\n");
    }
    free(list);

}*/