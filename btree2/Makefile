TEST_BTREE = test_btree

TEST_BTREE_OBJET = \
					btree.o\
					test_btree.o\

CFLAGS +=  -Wall -std=gnu99 -g

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_BTREE)

$(TEST_BTREE): $(TEST_BTREE_OBJET) 
	$(CC) -o $(TEST_BTREE) $(TEST_BTREE_OBJET) -lkdc_queue -lkdc_dblinked

clean:
	$(RM) $(TEST_BTREE) *.o
