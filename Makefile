CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LFLAGS = 

.PHONY: all clean

all: test

test: test.o jmalloc.o
	$(CC) -o test test.o jmalloc.o $(LFLAGS)

test.o: test.c jmalloc.h
	$(CC) $(CFLAGS) -c test.c

jmalloc.o: jmalloc.c
	#$(CC) $(CFLAGS) -DNDEBUG -c jmalloc.c # for use - removes debug printing
	$(CC) $(CFLAGS) -c jmalloc.c

clean:
	rm -f test test.o jmalloc.o
