.phony all:
all: mts

mts: mts.c
	gcc mts.c -Wall -pthread -lm train_node.c station_queue.c -o mts


.PHONY clean:
clean:
	-rm -rf *.o *.exe mts