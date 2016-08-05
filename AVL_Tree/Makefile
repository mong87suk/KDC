.SUFFIXES: .c.o

TREE = tree

TREE_LIBRARY = libkdc_tree.so

INSTALL_LIB_PATH = /usr/lib/
INSTALL_INCLUDE_PATH = /usr/include/kdc/

TEST_TREE = test_tree

TEST_TREE_OBJET = \
				   tree.o\
				   test_tree.o\

CFLAGS += -Wall -fpic -std=gnu99 -g -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_TREE) $(TREE_LIBRARY)

$(TEST_TREE): $(TEST_TREE_OBJET) 
	$(CC) -o $(TEST_TREE) $(TEST_TREE_OBJET) -lkdc_queue -lkdc_dblinked -lglib-2.0

$(TREE_LIBRARY): tree.o
	$(CC) -shared -o $@ $^

install:
	cp *.so $(INSTALL_LIB_PATH)
	mkdir -p $(INSTALL_INCLUDE_PATH)
	cp *.h $(INSTALL_INCLUDE_PATH)

uninstall:
	$(RM) $(INSTALL_LIB_PATH)$(TREE_LIBRARY) $(INSTALL_INCLUDE_PATH)$(TREE).h

clean:
	$(RM) $(TEST_TREE) *.o *.so
