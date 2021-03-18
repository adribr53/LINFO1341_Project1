#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lib/segment/packet_interface.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int setup() {
    return  0;
}

int teardown() {
    return 0;
}

void test_encode_decode() {    
    const char toEncode[]= {0x5c, 0x00, 0x0b, 0x7b, 0x17, 0x00, 0x00, 0x00, 0x4e, 0xa0, 0x77, 0xdb, 0x68, 0x65, 0x6c, 0x6c,
                     0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x0d, 0x4a, 0x11, 0x85}; // example from Inginious

    char forDecode[27]={0};
    pkt_t *testPkt=pkt_new();
    pkt_decode(toEncode, 27, testPkt);

    // getters tested for simple values (endianness not involved)
    CU_ASSERT_EQUAL(PTYPE_DATA, pkt_get_type(testPkt));
    CU_ASSERT_EQUAL(0, pkt_get_tr(testPkt)); 
    CU_ASSERT_EQUAL(28, pkt_get_window(testPkt));
    CU_ASSERT_EQUAL(0x7b, pkt_get_seqnum(testPkt));
    
    unsigned int *len = malloc(sizeof(unsigned int));
    *len=27;
    CU_ASSERT_EQUAL(PKT_OK ,pkt_encode(testPkt, forDecode, (size_t *) len))
    
    for (int i=0; i<27; i++) { // 
        CU_ASSERT_EQUAL(toEncode[i], forDecode[i]);       
    }
    free(len);
}

void test_decode_encode_tr() {
    pkt_t *testPkt=pkt_new();
    pkt_set_type(testPkt, PTYPE_DATA); 
    pkt_set_tr(testPkt, 1); // to check that encode doesn't change it
    pkt_set_window(testPkt, 28);
    pkt_set_seqnum(testPkt, 0x7b); 
    pkt_set_timestamp(testPkt, 0x17);
    pkt_set_payload(testPkt, "hello world", 11); //42+16=57

    unsigned int *len = malloc(sizeof(unsigned int));
    *len=57;
    char forEncode[57]={0};
    CU_ASSERT_EQUAL(PKT_OK, pkt_encode(testPkt, forEncode, (size_t *) len));
    
    pkt_t *forDecode=pkt_new();
    CU_ASSERT_EQUAL(PKT_OK, pkt_decode(forEncode, 57, forDecode));
    CU_ASSERT_EQUAL(PTYPE_DATA, pkt_get_type(forDecode));
    CU_ASSERT_EQUAL(1, pkt_get_tr(forDecode));
    CU_ASSERT_EQUAL(28, pkt_get_window(forDecode));
    CU_ASSERT_EQUAL(0x7b, pkt_get_seqnum(forDecode));
    CU_ASSERT_EQUAL(0x17, pkt_get_timestamp(forDecode));
    CU_ASSERT_EQUAL(11, pkt_get_length(forDecode));    
    pkt_del(testPkt);
    pkt_del(forDecode);
    free(len);
}


int main(int argc, char const *argv[]) {
    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("Suite", setup, teardown);    
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    if ((NULL == CU_add_test(pSuite, "Test encode_decode", test_encode_decode)) ||
        (NULL == CU_add_test(pSuite, "Test decode_encode_tr", test_decode_encode_tr))) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());
    CU_cleanup_registry();
    return 0; 
}
