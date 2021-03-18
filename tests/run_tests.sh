# Note that this assumes to be called from the Makefile, you may want to adapt it.

echo "Test of the packet API"
rm test_packet
gcc -o test_packet ./src/lib/segment/packet_implem.c ./tests/packet_test.c -lcunit -lz
./test_packet

echo "Test of the queue (window sender)"
rm test_queue
gcc -o test_queue ./src/lib/segment/packet_implem.c ./src/lib/linkedlist/linkedlist.c ./tests/send_window_test.c -lcunit -lz
./test_queue

echo "Test of the sorted list (window receiver)"
rm test_sorted_list
gcc -o test_sorted_list ./src/lib/segment/packet_implem.c ./src/lib/sortedLinkedList/sortedLinkedList.c ./tests/rec_window_test.c -lcunit -lz
./test_sorted_list

echo "Test of the program via link_sim"
cd tests 
./test0.sh
./test1.sh
./test2.sh
./test3.sh
./test4.sh
./test5.sh
./test6.sh
./test7.sh
./test8.sh
./test9.sh
