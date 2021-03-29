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

/*
 * Returns a pointer to a new struct list that is empty
 */
queue *new_list();
/*
 * Free the list
 */
void del_list(queue *list);

/*
 * Adds pkt to list
 * list : the list (prev init with new_list)
 * packet : ptr to the pkt to add (should not be null)
 */
int add(queue *list, pkt_t *packet);

/*
 * Returns the first elem of the list without remove it
 * list : the list (prev init with new_list)
 */
pkt_t *peek(queue* list);

/*
 * Removes the first elem of the list
 * list : the list (prev init with new_list)
 */
int dequeue(queue *list);

/*
 * Returns true if the list is empty
 * list : the list (prev init with new_list)
 */
int is_empty(queue *list);

/*
 * Returns if the seqnum is greater or equal to the smallest element of the list
 * seqnum : seqnum to evaluate
 * list : the list (prev init with new_list)
 */
int is_higher_or_equal(int seqnum, queue *list);

/*
 * Gets the packet with a specific seqnum
 * list : the list (prev init with new_list)
 * receive_seqnum : seqnum to get
 * tmpPtk : on return will be pointed to the getted pkt
 * supportIt : pointer to an int to know which element to look at next (avoids get() in O(n)) 
 */
int get(queue *list, uint8_t seqnum, pkt_t** resp, int *supportIt);

/*
 * Helps during print debugging sessions
 */
void print_seqnums(queue *list);

#endif //LINFO1341_PROJECT1_QUEUE_H
