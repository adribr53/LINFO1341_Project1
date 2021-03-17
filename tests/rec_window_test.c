#include <CUnit/CUnit.h>
#include <CUnit/Basic.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lib/segment/packet_interface.h"
#include "../src/lib/sortedLinkedList/sortedLinkedList.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

list_t *list;
int setup(){
    system("gcc -Wall -Werror -o packet_test ../src/lib/segment/packet_implement.c ../src/lib/sortedLinkedList/sortedLinkedList.h -lz");
    list=new_list();
    return  0;
}

int teardown() {
    free(list);
    system("rm test_window_send"); 
    return 0;
}

void test_basic() {
    
}