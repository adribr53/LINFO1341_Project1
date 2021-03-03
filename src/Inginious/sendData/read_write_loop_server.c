#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include "read_write_loop_server.h"

void read_write_loop_server(const int sfd, const int outfd) {

    struct pollfd sfdPoll;
    sfdPoll.fd=sfd;
    sfdPoll.events=POLLIN;

    char buf[1024];
    int nb;
    int err;
    while (1) {
        poll(&sfdPoll, 1 , -1);
        if (sfdPoll.revents & POLLIN) {
            //read(socket) -> write(stdout) ou send(othersocket)
            nb=read(sfd, buf, 1024);
            if (nb==0) return;
            err=write(outfd, buf, nb);
            if (err<0) return;
        }
    }

}
