TEST_BTREE = test_btree

TEST_BTREE_OBJET = \
					btree.o\
					test_btree.o\

TEST_GHASH = test_ghash

TEST_GHASH_OBJECT = g_hash_test.o

CFLAGS +=  -Wall -std=gnu99 -g -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_BTREE) $(TEST_GHASH)

$(TEST_BTREE): $(TEST_BTREE_OBJET) 
	$(CC) -o $(TEST_BTREE) $(TEST_BTREE_OBJET) -lkdc_queue -lkdc_dblinked

$(TEST_GHASH): $(TEST_GHASH_OBJECT)
	$(CC) -o $(TEST_GHASH) $(TEST_GHASH_OBJECT) -lglib-2.0

clean:
	$(RM) $(TEST_BTREE) *.o
