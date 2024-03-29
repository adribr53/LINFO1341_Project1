#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "log.h"
#include "lib/send_data/create_socket.h"
#include "lib/send_data/read_write_loop_sender.h"
#include "lib/send_data/real_address.h"

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *receiver_ip = NULL;
    char *receiver_port_err;
    uint16_t receiver_port;

    while ((opt = getopt(argc, argv, "f:s:h")) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }


    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    receiver_ip = argv[optind];
    receiver_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    ERROR("Sender has following arguments: filename is %s, stats_filename is %s, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, receiver_ip, receiver_port);

    int infile_fd;
    if (filename == NULL || strcmp(filename, "stdin") == 0) {
        if (filename == NULL) printf("Not filename found => set as STDIN\n");
        infile_fd = 0;
    } else {
        infile_fd = open(filename, O_RDONLY);
        // ERROR("INFILE => %d\n", infile_fd);
        if (infile_fd < -1){
            ERROR("Error on reading input");
            return EXIT_FAILURE;
        }
    }
    FILE* stats_file = NULL;

    if (stats_filename != NULL)
        stats_file = fopen(stats_filename, "w");

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    // ERROR("This is not an error, %s", "now let's code!");

    // Now let's code!

    // Socket() & Connect()
    if (strcmp("localhost", receiver_ip)==0) {
        strcpy(receiver_ip, "::1");
    }
    struct sockaddr_in6 receiver_addr;
    if (real_address(receiver_ip, &receiver_addr) != NULL) {
        ERROR("Error with the address");
        return EXIT_FAILURE;
    }

    int sfd = create_socket(NULL, -1, &receiver_addr, receiver_port);
    if (sfd < 0) {
        ERROR("Error while creating socket\n");
        return EXIT_FAILURE;
    }
    // Loop (Selective Repeat & close when finished)
    read_write_loop_sender(sfd, infile_fd, stats_file);

    close(sfd);
    if (infile_fd!=0) close(infile_fd);
    if (stats_file != NULL)
        fclose(stats_file);
    return EXIT_SUCCESS;
}