#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include "read_write_loop.h"

void read_write_loop(const int sfd) {

    struct pollfd pfd[2];

    struct pollfd stdinPoll;
    stdinPoll.fd=0;
    stdinPoll.events=POLLIN;

    struct pollfd sockPoll;
    sockPoll.fd=sfd;
    sockPoll.events=POLLIN;

    pfd[0]=stdinPoll;
    pfd[1]=sockPoll;

    char buf[1024];
    int nb;
    int err;
    while (1) {
        poll(pfd, 2 , -1);
        if (pfd[0].revents & POLLIN) {
            // read(stdin) -> write(socket)
            nb=read(0, buf, 1024);
            if (nb==0) return;
            err=write(sfd, buf, nb);
            if (err<0) return;
        }
        if (pfd[1].revents & POLLIN) {
            // read(socket) -> write(stdout)
            nb=read(sfd, buf, 1024);
            err=write(1, buf, nb);
            if (err<0) return;
        }
    }

}