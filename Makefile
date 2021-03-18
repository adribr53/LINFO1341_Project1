# You can use clang if you prefer
CC = gcc

# Feel free to add other C flags
CFLAGS += -c -std=gnu99 -Wall -Werror -Wextra -O2
# By default, we colorize the output, but this might be ugly in log files, so feel free to remove the following line.
CFLAGS += -D_COLOR

# You may want to add something here
LDFLAGS += "-lz"

# Adapt these as you want to fit with your project
SENDER_SOURCES = $(wildcard src/sender.c src/log.c src/lib/sendData/real_address.c src/lib/sendData/create_socket.c src/lib/sendData/read_write_loop_sender.c src/lib/segment/packet_implem.c src/lib/linkedlist/linkedlist.c)
RECEIVER_SOURCES = $(wildcard src/receiver.c src/log.c src/lib/sendData/create_socket.c src/lib/sendData/wait_for_client.c src/lib/sendData/real_address.c src/lib/sendData/read_write_loop_server_v2.c src/lib/segment/packet_implem.c src/lib/sortedLinkedList/sortedLinkedList.c)

SENDER_OBJECTS = $(SENDER_SOURCES:.c=.o)
RECEIVER_OBJECTS = $(RECEIVER_SOURCES:.c=.o)

SENDER = sender
RECEIVER = receiver

<<<<<<< HEAD
all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_OBJECTS)
=======
all: $(SENDER) $(RECEIVER) sim_link ## Build Sender and Receiver with all dependencies

# ====================================================Help============================================================
# code found here : https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html

help:
	@grep -E '(^[a-zA-Z_-]+:.*?##.*$$)|(^##)' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[32m%-20s\033[0m %s\n", $$1, $$2}' | sed -e 's/\[32m##/[33m/'

# ====================================================================================================================

$(SENDER): $(SENDER_OBJECTS) ## Build Sender compiled files and dependencies
>>>>>>> 39137c38088770d3dc48519cd31ae46ab0960676
	$(CC) $(SENDER_OBJECTS) -o $@ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_OBJECTS)
	$(CC) $(RECEIVER_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean mrproper

clean:
	rm -f $(SENDER_OBJECTS) $(RECEIVER_OBJECTS)
	rm receiver
	rm sender

mrproper:
	rm -f $(SENDER) $(RECEIVER)

# It is likely that you will need to update this
tests: all
	./tests/run_tests.sh

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += -D_DEBUG
debug: clean all

# Place the zip in the parent repository of the project
ZIP_NAME="../projet1_gaudin_giot.zip"

# A zip target, to help you have a proper zip file. You probably need to adapt this code.
zip:
	# Generate the log file stat now. Try to keep the repository clean.
	# git log --stat > gitlog.stat
	zip -r $(ZIP_NAME) Makefile src tests rapport.pdf gitlog.stat
	# We remove it now, but you can leave it if you want.
	# rm gitlog.stat
