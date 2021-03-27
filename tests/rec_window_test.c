#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lib/segment/packet_interface.h"
#include "../src/lib/sorted_list/sorted_list.h"
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
    pkt_t *test=pkt_new();
    if (test==NULL) return;
    pkt_set_seqnum(test, 0);
    pkt_set_payload(test, "br", 2);
    add(list, test, 0);
    CU_ASSERT_EQUAL(1, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);
   
    pkt_set_seqnum(test, 1);
    add(list, test, 0);
    CU_ASSERT_EQUAL(2, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_set_seqnum(test, 2);
    add(list, test, 0);    
    CU_ASSERT_EQUAL(3, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_set_seqnum(test, 3);
    add(list, test, 0);
    CU_ASSERT_EQUAL(4, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_set_seqnum(test, 4);
    add(list, test, 0);
    CU_ASSERT_EQUAL(5, list->size);
    CU_ASSERT_EQUAL(0, peek(list)->seqnum);

    pkt_set_seqnum(test, 5);
    add(list, test, 0);
    pkt_del(test);    
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