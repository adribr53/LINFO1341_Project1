#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lib/segment/packet_interface.h"
#include "../src/lib/linkedlist/linkedlist.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

linkedList *list;
int setup(){
    list=new_list();
    return  0;
}

int teardown() {
    free(list);
    return 0;
}

void test_case() {
    // ADD PACKETS
    pkt_t *test0=pkt_new();
    pkt_set_seqnum(test0, 0);
    pkt_set_payload(test0, "br0", 3);
    add(list, test0);
    CU_ASSERT_EQUAL(1, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1, 1);
    pkt_set_payload(test1, "br1\n", 3);
    add(list, test1);
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    pkt_t *test2=pkt_new();
    pkt_set_seqnum(test2, 2);
    pkt_set_payload(test2, "br2\n", 3); 
    add(list, test2);
    CU_ASSERT_EQUAL(3, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3, 3);
    pkt_set_payload(test3, "br3\n", 3);
    add(list, test3);
    CU_ASSERT_EQUAL(4, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4, 4);
    pkt_set_payload(test4, "br4\n", 3);
    add(list, test4);
    CU_ASSERT_EQUAL(5, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    pkt_t *test5=pkt_new();
    pkt_set_seqnum(test5, 5);
    pkt_set_payload(test5, "br5\n", 3);
    add(list, test5);
    CU_ASSERT_EQUAL(6, list->size);
    CU_ASSERT_EQUAL(0, pkt_get_seqnum(peek(list)));

    // part of rw_loop_sender.c : window changed due to ack
    // ACK 3 received
    int i=1;
    while (is_higher_or_equal(3, list)) {
        pop(list);
        CU_ASSERT_EQUAL(i++, pkt_get_seqnum(peek(list)));
    }
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(4, pkt_get_seqnum(peek(list)));
    printf("bonjour\n");
    // part of rw_loop_sender.c : window unchanged despite an ack
    // ACK 3 received, again
    while (is_higher_or_equal(3, list)) {
        pop(list);
        CU_ASSERT_EQUAL(i++, pkt_get_seqnum(peek(list)));
    }
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(4, pkt_get_seqnum(peek(list)));
    
    // part of rw_loop_sender.c : iterate over the window to resend some packets, according to the rtt
    int *supportInt=malloc(sizeof(int));
    *supportInt=-1; // outil pour itérer
    pkt_t *tmpPtk = peek(list);
    uint8_t receive_seqnum = pkt_get_seqnum(tmpPtk);
    while (get(list, receive_seqnum, &tmpPtk, supportInt)) {
        // Set new timestamp
        pkt_set_timestamp(tmpPtk, (uint32_t) 3);
        receive_seqnum=(receive_seqnum+1)%MAX_WINDOW_SIZE;
    }
    CU_ASSERT_EQUAL(4, pkt_get_seqnum(peek(list)));
    while (is_higher_or_equal(3, list)) {
        pop(list);
        CU_ASSERT_EQUAL(i++, pkt_get_seqnum(peek(list)));
    }
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(4, pkt_get_seqnum(peek(list)));

    free(supportInt);

    // Let's empty the list
    i=4;
    while (is_higher_or_equal(5, list))
    {
        CU_ASSERT_EQUAL(i++, pkt_get_seqnum(peek(list)));
        pop(list);
    }
    
}

int main(int argc, char const *argv[]) {
    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("Suite", setup, teardown);    
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if (NULL == CU_add_test(pSuite, "Test test_case", test_case)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());
    CU_cleanup_registry();
    return 0; 
}
