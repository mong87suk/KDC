CC=gcc
CFLAGS=-Wall -std=gnu99 -g

test: testBTree

testBTree: btree.o test_btree.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) testBTree *.o