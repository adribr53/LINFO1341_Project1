#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include "wait_for_client.h"

int wait_for_client(int sfd) {
    struct sockaddr_in6 peerAddr;
    socklen_t peerAddrLen = sizeof(struct sockaddr_in6);
    char buffer[1024];  // allocate a buffer of MAX_MESSAGE_SIZE bytes on the stack
    memset(&peerAddr, 0, sizeof(struct sockaddr));

    ssize_t n_received = recvfrom(sfd, buffer, 1024, MSG_PEEK, (struct sockaddr *) &peerAddr, &peerAddrLen);
    if (n_received==-1) return -1;
    n_received=connect(sfd, (struct sockaddr *)&peerAddr, peerAddrLen);
    if (n_received==-1) return -1;
    return 0;
}