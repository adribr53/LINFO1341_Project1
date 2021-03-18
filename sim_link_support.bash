#!/usr/bin/env bash

valgrind --log-file=valgrindReceiver --leak-check=full ./receiver :: 64341 1>testpog.txt &
./link_sim -p 64342 -P 64341 -e 40 -d 20 -j 20 -c 25 -l 35 -s 37909724617 &
valgrind --log-file=valgrindSender --leak-check=full ./sender -f $1 ::1 64342 &
