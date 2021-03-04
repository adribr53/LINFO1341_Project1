#include <stdint.h>
#include "../segment/packet_interface.h"

typedef struct node {
  struct node *next_t;
  pkt_t *packet_t;
} node_t;


typedef struct list {
  struct node *first_t;
} list_t;

list_t *new_list();
node_t *new_node(pkt_t *packet);
int add(list_t *list, pkt_t *packet);
pkt_t* peek(list_t* list);
pkt_t* pop(list_t *list);
int is_empty(list_t *list);

