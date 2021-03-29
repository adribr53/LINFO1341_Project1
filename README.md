# LINFO1341 Project - Giot Adrien & Gaudin Félix

## How to run it ?

There is a Makefile at the root of the projet.
```bash
make (all) # Build Sender and Receiver with all dependencies

make clean # Remove all compilled file

make tests # Launch our tests

make zip # Generate zip and log file

make help # Show help for makefile
```
## Description 
* For a detailed description of how our program works : see rapport.pdf
* To access the specifications of the functions : see headers (.h)
* "make tests" takes some time to be completed (3 minutes (Intel room))
 
## Ressources used for this project
* [Stackoverflow](https://stackoverflow.com/)
* [Man pages](https://man7.org/linux/man-pages/)
* [The syllabus](https://beta.computer-networking.info/syllabus/default/index.html)

## Dependencies
*All dependencies path is relative to src file*

### Sender
* log.h 
* lib/send_data/real_address.h
* lib/send_data/create_socket.h
* lib/send_data/read_write_loop_sender.h (main core of the sender)
* lib/segment/packet_interface.h
* lib/queue/queue.h
* lib/stats/stats.h

### Receiver
* log.h
* lib/send_data/real_address.h
* lib/send_data/create_socket.h
* lib/send_data/wait_for_client.h
* lib/send_data/read_write_loop_server_v2.h (main core of the receiver)
* lib/segment/packet_interface.h
* lib/sorted_list/sorted_list.h
* lib/stats/stats.h

## Special thanks
![My cat](https://media.discordapp.net/attachments/669931364149100554/795971633105600522/124042033_362655141621936_584480919917219971_n.jpg?width=250&height=250)
