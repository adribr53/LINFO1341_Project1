//
// Created by felix on 3/03/21.
//
#include <poll.h> // poll
#include <unistd.h> // read

#include "read_write_loop_sender.h"

#define TRUE 1
#define FALSE 0

void read_write_loop_sender(const int sfd, const int input_fd) {
    struct pollfd pfd[2];
    // 0 => socket
    // 1 => sender input

    struct pollfd sockPoll;
    sockPoll.fd=sfd;
    sockPoll.events=POLLIN;

    struct pollfd inputPoll;
    inputPoll.fd=input_fd;
    inputPoll.events=POLLIN;

    pfd[0]=sockPoll;
    pfd[1]=inputPoll;

    char buf[1024];
    int nb;
    int err;
    // int waitForAck = FALSE;
    while (1) {
        poll(pfd, 2 , -1);
        if (pfd[0].revents & POLLIN) {
            // read(stdin) -> write(socket)
            nb=read(input_fd, buf, 1024);
            if (nb==0) return;
            err=write(sfd, buf, nb);
            if (err<0) return;
            // waitForAck = TRUE;
        }
        if (pfd[1].revents & POLLIN) {
            // read(socket) -> write(stdout)
            nb=read(sfd, buf, 1024);
        }
    }
}