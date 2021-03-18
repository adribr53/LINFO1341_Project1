#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "./lib/sendData/real_address.h"
#include "./lib/sendData/create_socket.h"
#include "./lib/sendData/wait_for_client.h"
#include "./lib/sendData/read_write_loop_server_v2.h"

int sock;

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}

int connection_to_sender(char* listen_ip, uint16_t listen_port) {
    struct sockaddr_in6 addressSock;

    const char *err = real_address(listen_ip, &addressSock);
    if (err!=NULL) {
        fprintf(stderr, "Adress unrecognized, error message : %s\n", err);
        return 1;
    }

    sock = create_socket(&addressSock, listen_port, NULL, -1);
    if (sock < 0) {
        fprintf(stderr, "Error during the execution of create_socket()");
        return 1;
    }

    if (wait_for_client(sock)==-1) {
        fprintf(stderr, "Error during the execution of wait_for_client()");
        return 1;
    }

    return 0;
}


int main(int argc, char **argv) {
    int opt;

    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }
    if (stats_filename != NULL) fprintf(stderr, "COUCOU\n");
    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    listen_ip = argv[optind];
    if (strcmp("localhost", listen_ip)==0) {
        fprintf(stderr, "test local");
        strcpy(listen_ip, "::");
    }

    listen_port = (uint16_t) strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    //ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
    //    stats_filename, listen_ip, listen_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    //ERROR("This is not an error, %s", "now let's code!");

    // Now let's code!
    if (connection_to_sender(listen_ip, listen_port)!=0) {
        fprintf(stdout, "Error during the execution of connection_to_sender");
        return EXIT_FAILURE;
    }
    // printf("first\n");
    read_write_loop_server(sock, 1);

    return EXIT_SUCCESS;
}
