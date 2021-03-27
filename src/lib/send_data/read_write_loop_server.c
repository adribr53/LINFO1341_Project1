#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_write_loop_server.h"
#include "../segment/packet_interface.h"
#include <sys/socket.h>
#include "../sorted_list/sorted_list.h"
#include "../stats/stat.h"

#define ACK_PKT_SIZE 10

int in_window(uint8_t waited, uint8_t received) {
    if(received >= waited){ 
		 return received-waited<=32;
	}
    return 256+received-waited<=32;

}

int build_ack(pkt_t* pkt, uint32_t timestamp, uint8_t seqnum, ptypes_t type) {
    //fprintf(stderr, "ack sent of num %d\n", seqnum);
    pkt_set_type(pkt, type);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, MAX_WINDOW_SIZE);
    pkt_set_seqnum(pkt, seqnum);
    pkt_set_length(pkt, 0);
    pkt_set_timestamp(pkt, timestamp);
    return 0;
}

void read_write_loop_server(const int sfd, const int outfd, FILE* stat_file) {
    // Init variables
    struct pollfd sfdPoll;
    sfdPoll.fd=sfd;
    sfdPoll.events=POLLIN;

    list_t* window=new_list(); // TODO: free

    char data_buffer[MAX_PAYLOAD_SIZE+16];
    char buffer[ACK_PKT_SIZE];

    int nb;
    size_t ack_size = (size_t) 10;
    uint8_t waitedSeqnum = 0; // uint8 so no need to take care of 0-1 and 255+1
    uint64_t receivePkt = 0;
    
    pkt_t *pkt=pkt_new(); // TODO: free

    logger* receiver_logger = new_logger(0, stat_file);

    while (1) {
        int isRunning=poll(&sfdPoll, 1 , 3000);
        if (isRunning == 0) { // Timeout
            fprintf(stderr, "receiver terminates\n");
            del_list(window);
            remove_logger(receiver_logger);
            return;
        }
        if (sfdPoll.revents && POLLIN) {
            // Data on sfd
            nb=recv(sfd, data_buffer, MAX_PAYLOAD_SIZE+16, 0);
            if (nb==0) return;
            // Decode pkt
            if (pkt_decode(data_buffer, nb, pkt) == PKT_OK) {
                switch ((int) pkt_get_type(pkt)) {
                    case 1:
                        // DATA
                        add_data_received(receiver_logger);
                        if (pkt_get_tr(pkt)==1) {
                            // Truncate pkt
                            add_data_truncated_received(receiver_logger);
                            pkt_t* ack_pkt = pkt_new();
                            build_ack(ack_pkt, pkt_get_timestamp(pkt), waitedSeqnum, PTYPE_NACK);
                            pkt_encode(ack_pkt, buffer, &ack_size);
                            nb = send(sfd, buffer, ack_size, MSG_CONFIRM);
                            if (nb == 0) {
                                fprintf(stderr, "Error while resending ACK\n");
                                return;
                            }
                            add_nack_sent(receiver_logger);
                            pkt_del(ack_pkt);
                        }
                        else
                            // Untruncate pkt
                        if (pkt_get_seqnum(pkt) == waitedSeqnum) {
                            // [Good seqnum] received (data.ind it and the consecutive seqnum)
                            if (pkt_get_length(pkt)==0) {
                                // Disconnect received
                                fprintf(stderr, "receiver terminates, due to a message\n");
                                // Free
                                del_list(window);
                                if (pkt != NULL) pkt_del(pkt);
                                remove_logger(receiver_logger);
                                return;
                            }
                            fprintf(stderr, "seqnum waited arrived : %d \n", waitedSeqnum);
                            // parse window
                            int oldSeqnum=waitedSeqnum;
                            nb=write(outfd, pkt_get_payload(pkt), pkt_get_length(pkt));
                            if (nb == 0) {
                                fprintf(stderr, "Error while writing in outfile\n");
                                return;
                            }
                            uint32_t timestampAck=pkt_get_timestamp(pkt);
                            waitedSeqnum=(waitedSeqnum+1)%256;
                            receivePkt=1;
                            // Unfill the window with consecutive seqnum (first pkt was not in the window)
                            while (!is_empty(window) && waitedSeqnum==peek(window)->seqnum) {
                                if (peek(window)->length == 0) {
                                    // disconnect
                                    fprintf(stderr, "Receiver terminates");
                                    // Free
                                    del_list(window);
                                    if (pkt != NULL) pkt_del(pkt);
                                    remove_logger(receiver_logger);
                                    return;
                                }
                                nb=write(outfd, peek(window)->payload, peek(window)->length);
                                if (nb==0) {
                                    fprintf(stderr, "Error while writing in outfile\n");
                                    return;
                                }
                                timestampAck=peek(window)->timestamp;
                                pop(window);
                                waitedSeqnum=(waitedSeqnum+1)%256;
                                receivePkt++;
                            }
                            reset(window, waitedSeqnum, oldSeqnum);

                            // Send ack
                            pkt_t* ack_pkt = pkt_new();
                            build_ack(ack_pkt, timestampAck, waitedSeqnum, PTYPE_ACK);
                            pkt_encode(ack_pkt, buffer, &ack_size);
                            nb = send(sfd, buffer, ack_size, MSG_CONFIRM);
                            if (nb == 0) {
                                fprintf(stderr, "Error while sending ACK\n");
                                return;
                            }
                            add_ack_sent(receiver_logger);
                            pkt_del(ack_pkt);
                        } else {
                            // Unwaited seqnum
                            if (in_window(waitedSeqnum, pkt_get_seqnum(pkt))) {
                                // Store in window
                                add_data_received(receiver_logger);
                                switch (add(window, pkt, waitedSeqnum)) {
                                    case -1:
                                        // Error
                                        fprintf(stderr, "Error while adding to the window\n");
                                        return;
                                        break;
                                    case 1:
                                        // Duplicated => ignored
                                        add_packet_ignored(receiver_logger);
                                        add_packet_duplicated(receiver_logger);
                                        break;
                                    // case 0 => all ok, nothing to do
                                };
                            } else {
                                // Not in window => ignore
                                add_packet_ignored(receiver_logger);
                            }
                            if (receivePkt != 0) {
                                // Resend last ack pkt
                                pkt_t* ack_pkt = pkt_new();
                                build_ack(ack_pkt, pkt_get_timestamp(pkt), waitedSeqnum, PTYPE_ACK);
                                pkt_encode(ack_pkt, buffer, &ack_size);
                                nb = send(sfd, buffer, ack_size, MSG_CONFIRM);
                                if (nb == 0) {
                                    fprintf(stderr, "Error while resending ACK\n");
                                    return;
                                }
                                add_ack_sent(receiver_logger);
                                pkt_del(ack_pkt);
                            }
                        }
                        break;
                    case 2:
                        // ACK
                        add_ack_received(receiver_logger);
                        break;
                    case 3:
                        // NACK
                        add_nack_received(receiver_logger);
                        break;
                }
            } else {
                // ignored pkt
                add_packet_ignored(receiver_logger);
            }
        }
    }
}