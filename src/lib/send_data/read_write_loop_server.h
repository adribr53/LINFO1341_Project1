#ifndef LINFO1341_PROJECT1_READ_WRITE_LOOP_SERVER_V2_H
#define LINFO1341_PROJECT1_READ_WRITE_LOOP_SERVER_V2_H

/* Main function of the receiver 
 * This is where we implement the protocol itself
 */
void read_write_loop_server(const int sfd, const int outfd, FILE* stat_file);

#endif //LINFO1341_PROJECT1_READ_WRITE_LOOP_SERVER_V2_H
