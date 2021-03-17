#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "create_socket.h"

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port) {
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock<0) {
        fprintf(stderr, "socket(AF_INET6, SOCK_DGRAM, 0)");
        return -1;
    }

    int err;

    if (source_addr!=NULL && src_port>0) {
        //source_addr->sin6_family=AF_INET6;
        source_addr->sin6_port = htons(src_port);
        err = bind(sock, (struct sockaddr *) source_addr, sizeof(struct sockaddr_in6));  // assign our address to the socket
        if (err == -1) {
            fprintf(stderr, "Error during the execution of bind(sock, (SOCKADDR *) source_addr, sizeof(*source_addr))\n");
            return -1;
        }
    }

    if (dest_addr!=NULL && dst_port>0) {
        //est_addr->sin6_family=AF_INET6;
        dest_addr->sin6_port = htons(dst_port);
        err = connect(sock, (struct sockaddr *) dest_addr, sizeof(struct sockaddr_in6)); // connect the socket to the dest
        if (err == -1) {
            fprintf(stderr, "connect(sock, (SOCKADDR *) dest_addr, sizeof(*dest_addr))");
            return -1;
        }
    }
    return sock;

}