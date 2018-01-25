CC=gcc
CFLAGS=-O3 -g -Wall -Wpedantic -Wextra -std=c99
LDFLAGS=
EXECUTABLE=td-bs

all: td-bs

td-bs: td-bs.o graph.o stack.o bitset.o union_find.o
	$(CC) $(LDFLAGS) td-bs.o graph.o stack.o bitset.o union_find.o -o $(EXECUTABLE)

td-bs.o: td-bs.c graph.h bitset.h stack.h kbtree.h
	$(CC) $(CFLAGS) -c td-bs.c

graph.o: graph.c graph.h stack.h bitset.h
	$(CC) $(CFLAGS) -c graph.c

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c

bitset.o: bitset.c bitset.h
	$(CC) $(CFLAGS) -c bitset.c

union_find.o: union_find.c union_find.h
	$(CC) $(CFLAGS) -c union_find.c

clean:
	rm -f td-bs.o graph.o stack.o bitset.o union_find.o $(EXECUTABLE)

