CC=gcc
CFLAGS=-Wall -g

all: vsfsck

vsfsck: vsfsck.o utils.o
	$(CC) $(CFLAGS) -o vsfsck vsfsck.o utils.o

clean:
	rm -f *.o vsfsck
