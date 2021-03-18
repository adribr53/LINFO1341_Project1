#include <stdint.h>
#include "../segment/packet_interface.h"

typedef struct payload_seqnum_timestamp {
  char payload[MAX_PAYLOAD_SIZE];
  uint16_t length;
  uint8_t seqnum;
  uint32_t timestamp;
} pst_t;

typedef struct list {
  pst_t *window[MAX_WINDOW_SIZE];
  int size;
  uint8_t ptr;
} list_t;

int ind(uint8_t toPlace, uint8_t waited);
list_t *new_list();
void del_list(list_t *list);  
int add(list_t *list, pkt_t *packet, uint8_t waited);
pst_t *peek(list_t* list);
int pop(list_t *list);
int is_empty(list_t *list);
void reset(list_t *list, uint8_t newWaited, uint8_t oldWaited);

