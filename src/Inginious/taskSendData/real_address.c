#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> /* * sockaddr_in6 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "real_address.h"

const char * real_address(const char *address, struct sockaddr_in6 *rval) {
    if (address == NULL || rval == NULL) {
        return "NULL input";
    }

    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    struct addrinfo *result;

    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = 0;


    int err = getaddrinfo(address, NULL, &hints, &result);
    if (err!=0) return "error at getadrrinfo()";


    struct sockaddr_in6 *tmp = (struct sockaddr_in6 *)(result->ai_addr);
    *rval=*tmp;

    freeaddrinfo(result);
    return NULL;
}
