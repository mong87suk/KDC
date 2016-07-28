TEST_BTREE = test_btree

QUEUE = QUEUE

DBLINKED_LIST = DBLinkedList

PRE_INSTALL = PRE_INSTALL 	# Make library

UNINSTALL = UNINSTALL		# Delete library

MAKDE_DIR = MAKDE_DIR		# Make DIR

DEL_DIR = DEL_DIR 			# Delete Directory

HEADER = HEADER

QUEUE_SOURCE = ../chapter3/DBLinkedList/Queue.c

DBLINKED_LIST_SOURCE = ../chapter3/DBLinkedList/DBLinkedList.c

QUEUE_OBJECT = Queue.o

DBLINKED_LIST_OBJECT = DBLinkedList.o

TEST_QUEU_OBJET = \
					btree.o\
					test_btree.o\

SHARED_DBLINKED_LIST_LIBRARY = lib$(DBLINKED_LIST).so

SHARED_QUEUE_LIBRARY = lib$(QUEUE).so

CFLAGS +=  -Wall -std=gnu99 -g

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(TEST_BTREE)

$(TEST_BTREE): $(TEST_QUEU_OBJET) 
	$(CC) -o $(TEST_BTREE) $(TEST_QUEU_OBJET) -l$(QUEUE) -l$(DBLINKED_LIST)

install: $(PRE_INSTALL)
		
$(PRE_INSTALL): $(HEADER) $(SHARED_DBLINKED_LIST_LIBRARY) $(SHARED_QUEUE_LIBRARY)
	sudo cp $(SHARED_DBLINKED_LIST_LIBRARY) $(SHARED_QUEUE_LIBRARY) /usr/lib/

$(HEADER): $(MAKDE_DIR) 
	sudo cp ../chapter3/DBLinkedList/DBLinkedList.h ../chapter3/DBLinkedList/Queue.h /usr/include/kdc

$(MAKDE_DIR):
	sudo mkdir /usr/include/kdc

$(SHARED_DBLINKED_LIST_LIBRARY):$(DBLINKED_LIST_OBJECT)
	$(CC) -shared -o $@ $^

$(DBLINKED_LIST_OBJECT):
	$(CC) -c -Wall -fpic $(DBLINKED_LIST_SOURCE)

$(SHARED_QUEUE_LIBRARY):$(QUEUE_OBJECT)
	$(CC) -shared -o $@ $^

$(QUEUE_OBJECT):
	$(CC) -c -Wall -fpic $(QUEUE_SOURCE) -l$(DBLINKED_LIST)

clean: $(DEL_DIR) $(UNINSTALL) 
	$(RM) $(TEST_BTREE) *.o *.so

uninstall: $(UNINSTALL)

$(DEL_DIR):
	sudo rm -rf /usr/include/kdc

$(UNINSTALL):
	sudo rm /usr/lib/$(SHARED_DBLINKED_LIST_LIBRARY) /usr/lib/$(SHARED_QUEUE_LIBRARY)
