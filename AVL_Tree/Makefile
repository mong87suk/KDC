TEST_TREE = test_tree

TEST_TREE_OBJET = \
				   tree.o\
				   test_tree.o\

CFLAGS += -Wall -std=gnu99 -g

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_TREE)

$(TEST_TREE): $(TEST_TREE_OBJET) 
	$(CC) -o $(TEST_TREE) $(TEST_TREE_OBJET)

clean:
	$(RM) $(TEST_TREE) *.o