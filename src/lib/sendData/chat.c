#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#include "real_address.h"
#include "create_socket.h"
#include "read_write_loop.h"
#include "wait_for_client.h"

const char * real_address(const char *address, struct sockaddr_in6 *rval) {
    /* Resolve the resource name to an usable IPv6 address
     * @address: The name to resolve
     * @rval: Where the resulting IPv6 address descriptor should be stored
     * @return: NULL if it succeeded, or a pointer towards
     *          a string describing the error if any.
     *          (const char* means the caller cannot modify or free the return value,
     *           so do not use malloc!)
     * getaddrinfo, netinet_in.h
     * https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
     *  The hints argument points to an addrinfo structure that specifies
       criteria for selecting the socket address structures returned in
       the list pointed to by res.  If hints is not NULL it points to an
       addrinfo structure whose ai_family, ai_socktype, and ai_protocol
       specify criteria that limit the set of socket addresses returned
       by getaddrinfo()

     */
    if (address == NULL || rval == NULL) {
        return "NULL input";
    }

    struct addrinfo hints;
    struct addrinfo *result;

    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = 0;


    int err = getaddrinfo(NULL, address, &hints, &result);
    if (err!=0) return "error at getadrrinfo()";
    rval=(sockaddr_in6*) result->ai_addr;

    freeaddrinfo(result);
    return NULL;
}

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port) {
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock<0) {
        fprintf(stderr, "socket(AF_INET6, SOCK_DGRAM, 0)");
        return -1;
    }

    int err;

    if (source_addr!=NULL && src_port>0) {
        err = bind(sock, (SOCKADDR *) source_addr, sizeof(struct sockaddr_in6));  // assign our address to the socket
        if (err == -1) {
            fprintf(stderr, "bind(sock, (SOCKADDR *) source_addr, sizeof(*source_addr))");
            return -1;
        }
    }

    if (dest_addr!=NULL && dst_port>0) {
        err = connect(sock, (SOCKADDR *) dest_addr, sizeof(struct sockaddr_in6)); // connect the socket to the dest
        if (err == -1) {
            fprintf(stderr, "connect(sock, (SOCKADDR *) dest_addr, sizeof(*dest_addr))");
            return -1;
        }
    }
    return sock;

}

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(const int sfd) {
    struct pollfd stdinPoll;
    stdinPoll.fd=0;
    stdinPoll.events=POLLIN;
    char bufStd[1024];

    struct pollfd sockPoll={sfd, POLLIN};
    sockPoll.fd=sfd;
    sockPoll.events=POLLIN;
    char bufSock[1024];

    //poll(stdinPoll, 1, -1);
    //poll(sockPoll, 1, -1);

    while (1) {
        poll(stdinPoll, 1, -1);
        poll(sockPoll, 1, -1);
        if (stdinPoll.revents!=0) {
            if (fscanf(0,"%s",bufStd)==EOF) return;
            write(sfd, bufStd);
        }
        if (sockPoll.revents!=0) {
            read(sfd, bufSock, 1);
            write(stdout, bufSock);
        }
    }
}

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd) {

}

int main(int argc, char *argv[])
{
	int client = 0;
	int port = 12345;
	int opt;
	char *host = "::1";

	while ((opt = getopt(argc, argv, "scp:h:")) != -1) {
		switch (opt) {
			case 's':
				client = 0;
				break;
			case 'c':
				client = 1;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				host = optarg;
				break;
			default:
				fprintf(stderr, "Usage:\n"
								"-s      Act as server\n"
								"-c      Act as client\n"
								"-p PORT UDP port to connect to (client)\n"
								"        or to listen on (server)\n"
								"-h HOST UDP of the server (client)\n"
								"        or on which we listen (server)\n");
				break;
		}
	}
	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}
	/* Get a socket */
	int sfd;
	if (client) {
		sfd = create_socket(NULL, -1, &addr, port); /* Connected */
	} else {
		sfd = create_socket(&addr, port, NULL, -1); /* Bound */
		if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */
			fprintf(stderr,
					"Could not connect the socket after the first message.\n");
			close(sfd);
			return EXIT_FAILURE;
		}
	}
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	/* Process I/O */
	read_write_loop(sfd);

	close(sfd);

	return EXIT_SUCCESS;
}

