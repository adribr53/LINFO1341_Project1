# Note that this assumes to be called from the Makefile, you may want to adapt it.
echo "Test of the queue (window sender)"
rm test_queue
gcc -o test_queue ./src/lib/segment/packet_implem.c ./src/lib/linkedlist/linkedlist.c ./tests/rec_window_test.c -lcunit -lz
./test_queue

echo "Test of the packet API"
rm test_packet
gcc -o test_packet ./src/lib/segment/packet_implem.c ./tests/packet_test.c -lcunit -lz
./test_packet

echo "Test of the program via link_sim"
./tests/test0.sh
./tests/test1.sh
./tests/test2.sh
./tests/test3.sh
./tests/test4.sh
./tests/test5.sh
./tests/test6.sh
./tests/test7.sh
./tests/test8.sh
./tests/test9.sh
