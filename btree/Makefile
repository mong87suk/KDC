CC=gcc
CFLAGS=-Wall -std=gnu99 -g

test: testBTree

testBTree: bTree.o testBTree.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) testBTree *.o