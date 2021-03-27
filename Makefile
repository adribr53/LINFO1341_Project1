# You can use clang if you prefer
CC = gcc

# Feel free to add other C flags
CFLAGS += -c -std=gnu99 -Wall -Werror -Wextra -O2
# By default, we colorize the output, but this might be ugly in log files, so feel free to remove the following line.
CFLAGS += -D_COLOR

# You may want to add something here
LDFLAGS += "-lz"

# Adapt these as you want to fit with your project
SENDER_SOURCES = $(wildcard src/sender.c src/log.c src/lib/sendData/real_address.c src/lib/sendData/create_socket.c src/lib/sendData/read_write_loop_sender.c src/lib/segment/packet_implem.c src/lib/queue/queue.c src/lib/stats/stat.c)
RECEIVER_SOURCES = $(wildcard src/receiver.c src/log.c src/lib/sendData/create_socket.c src/lib/sendData/wait_for_client.c src/lib/sendData/real_address.c src/lib/sendData/read_write_loop_server_v2.c src/lib/segment/packet_implem.c src/lib/sortedList/sortedList.c src/lib/stats/stat.c)

SENDER_OBJECTS = $(SENDER_SOURCES:.c=.o)
RECEIVER_OBJECTS = $(RECEIVER_SOURCES:.c=.o)

SENDER = sender
RECEIVER = receiver

all: $(SENDER) $(RECEIVER) sim_link ## Build Sender and Receiver with all dependencies

# ====================================================Help============================================================
# code found here : https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html

help:
	@grep -E '(^[a-zA-Z_-]+:.*?##.*$$)|(^##)' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[32m%-20s\033[0m %s\n", $$1, $$2}' | sed -e 's/\[32m##/[33m/'

# ====================================================================================================================

$(SENDER): $(SENDER_OBJECTS) ## Build Sender compiled files and dependencies
	$(CC) $(SENDER_OBJECTS) -o $@ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_OBJECTS) ## Build Receiver compiled files and dependencies
	$(CC) $(RECEIVER_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean mrproper

clean: ## Remove all compilled file
	-rm -f $(SENDER_OBJECTS) $(RECEIVER_OBJECTS)
	-rm receiver
	-rm sender
	-rm link_sim
	-rm tests/input_file
	-rm tests/received_file

# It is likely that you will need to update this
tests: all ## Launch our tests
	./tests/run_tests.sh

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += -D_DEBUG
debug: clean all

# Place the zip in the parent repository of the project
ZIP_NAME="../projet1_gaudin_giot.zip"

# A zip target, to help you have a proper zip file. You probably need to adapt this code.
zip: ## Generate the log file stat now
	# git log --stat > gitlog.stat
	zip -r $(ZIP_NAME) Makefile src tests link_sim_repo rapport.pdf gitlog.stat README.md sim_link_support.bash
	# We remove it now, but you can leave it if you want.
	# rm gitlog.stat

sim_link: ## Generate link_sim compilled files
	gcc link_sim_repo/link_sim.c -o link_sim link_sim_repo/min_queue.c -std=c99 -Wall -Werror -Wshadow -Wextra -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -D_XOPEN_SOURCE -D_POSIX_C_SOURCE=201112L -rdynamic -lrt