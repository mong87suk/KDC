TEST_TREE = test_tree

TEST_TREE_OBJET = \
				   tree.o\
				   test_tree.o\

CFLAGS += -Wall -std=gnu99 -g -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_TREE)

$(TEST_TREE): $(TEST_TREE_OBJET) 
	$(CC) -o $(TEST_TREE) $(TEST_TREE_OBJET) -lkdc_queue -lkdc_dblinked -lglib-2.0

clean:
	$(RM) $(TEST_TREE) *.o