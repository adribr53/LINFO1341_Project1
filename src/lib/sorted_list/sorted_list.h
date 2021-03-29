#include <stdint.h>
#include "../segment/packet_interface.h"

typedef struct payload_seqnum_timestamp_length {
  char payload[MAX_PAYLOAD_SIZE];
  uint16_t length;
  uint8_t seqnum;
  uint32_t timestamp;
} pstl_t;

typedef struct list {
  pstl_t *window[MAX_WINDOW_SIZE];
  int size;
  uint8_t ptr;
} list_t;

/* Computes the index related to the seqnum toPlace 
 * based on the next expected seqnum (waited)
 */
int ind(uint8_t toPlace, uint8_t waited);

/* 
 * Creates the struct used to handle the window 
 */
list_t *new_list();

/* 
 * Free the struct used to handle the window
 */
void del_list(list_t *list);  

/* Inserts packet in list,
 * at the right position (computed by ind())
 * for the next expected seqnum (waited)
 */
int add(list_t *list, pkt_t *packet, uint8_t waited);

/*
 * Returns the element related to the smallest seqnum
 */
pstl_t *peek(list_t* list);

/*
 * Deletes the element related to the smallest seqnum
 */
int pop(list_t *list);

/*
 * Return 1 if list is empty (size==0), 0 if it's not
 */
int is_empty(list_t *list);

/*
 * Shifts the elements of the array to the left according to the diff between newWaited and oldWaited
 */
void reset(list_t *list, uint8_t newWaited, uint8_t oldWaited);

