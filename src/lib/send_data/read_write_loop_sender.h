//
// Created by felix on 3/03/21.
//

#ifndef LINFO1341_PROJECT1_READ_WRITE_LOOP_SENDER_H
#define LINFO1341_PROJECT1_READ_WRITE_LOOP_SENDER_H

/* Main function of the sender 
 * This is where we implement the protocol itself
 */
void read_write_loop_sender(const int sfd, const int input_fd, FILE* stats_file);

#endif //LINFO1341_PROJECT1_READ_WRITE_LOOP_SENDER_H
