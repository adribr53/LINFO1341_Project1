#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lib/segment/packet_interface.h"
#include "../src/lib/sortedList/sortedList.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

list_t *list;
int setup(){
    list=new_list();
    return  0;
}

int teardown() {
    free(list);
    return 0;
}

void test_basic() {
    pkt_t *test0=pkt_new();
    pkt_set_seqnum(test0, 0);
    pkt_set_payload(test0, "br0", 3);
    add(list, test0, 0);
    pkt_del(test0);
    CU_ASSERT_EQUAL(1, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1, 1);
    pkt_set_payload(test1, "br1\n", 3);
    add(list, test1, 0);
    pkt_del(test1);
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_t *test2=pkt_new();
    pkt_set_seqnum(test2, 2);
    pkt_set_payload(test2, "br2\n", 3); 
    add(list, test2, 0);
    pkt_del(test2);    
    CU_ASSERT_EQUAL(3, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3, 3);
    pkt_set_payload(test3, "br3\n", 3);
    add(list, test3, 0);
    pkt_del(test3);
    CU_ASSERT_EQUAL(4, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4, 4);
    pkt_set_payload(test4, "br4\n", 3);
    add(list, test4, 0);
    pkt_del(test4);
    CU_ASSERT_EQUAL(5, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_t *test5=pkt_new();
    pkt_set_seqnum(test5, 5);
    pkt_set_payload(test5, "br5\n", 3);
    add(list, test5, 0);
    pkt_del(test5);    
    CU_ASSERT_EQUAL(6, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    int i=0;
    while (peek(list)!=NULL) {
        CU_ASSERT_EQUAL(peek(list)->seqnum, i++);
        pop(list);
        CU_ASSERT_EQUAL(list->size, 6-i);
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
    if (NULL == CU_add_test(pSuite, "Test test_basic", test_basic)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());
    CU_cleanup_registry();
    return 0; 
}