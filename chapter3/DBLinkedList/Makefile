.SUFFIXES: .c.o

STACK = stack

QUEUE = queue

DBLINKED = dblinked

DBLINKED_LIBRARY = libdblinked

INSTALL_LIB_PATH = /usr/lib/
INSTALL_INCLUDE_PATH = /usr/include/kdc/

LINKED_LIBRARY = libkdc_dblinked.so
QUEUE_LIBRARY = libkdc_queue.so
STACK_LIBRARY = libkdc_stack.so

STACK_OBJECTS = \
		StackMain.o\
		Stack.o \
		DBLinkedList.o
DBLINKED_OBJECTS = \
		   DBLinkedListMain.o \
		   DBLinkedList.o
QUEUE_OBJECTS = \
		QueueMain.o\
		Queue.o\
		DBLinkedList.o
DBLINKED_LIBRARY_OBJECTS = \
		Queue.o\
		Stack.o\
		DBLinkedList.o

CFLAGS += -Wall -fpic -g

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(STACK) $(DBLINKED) $(QUEUE) $(LINKED_LIBRARY) $(QUEUE_LIBRARY) $(STACK_LIBRARY)

$(STACK): $(STACK_OBJECTS)
	$(CC) -o $(STACK) $(STACK_OBJECTS)

$(QUEUE): $(QUEUE_OBJECTS)
	$(CC) -o $(QUEUE) $(QUEUE_OBJECTS)

$(DBLINKED): $(DBLINKED_OBJECTS)
	$(CC) -o $(DBLINKED) $(DBLINKED_OBJECTS)

$(LINKED_LIBRARY): DBLinkedList.o
	$(CC) -shared -o $@ $^

$(QUEUE_LIBRARY): Queue.o
	$(CC) -shared -o $@ $^

$(STACK_LIBRARY): Stack.o
	$(CC) -shared -o $@ $^

install:
	cp *.so $(INSTALL_LIB_PATH)
	mkdir $(INSTALL_INCLUDE_PATH)
	cp *.h $(INSTALL_INCLUDE_PATH)

uninstall:
	$(RM) $(INSTALL_LIB_PATH)$(LINKED_LIBRARY) $(INSTALL_LIB_PATH)$(QUEUE_LIBRARY) $(INSTALL_LIB_PATH)$(STACK_LIBRARY) $(INSTALL_INCLUDE_PATH) -rf

clean:
	$(RM) -f *.o $(STACK) $(DBLINKED) $(QUEUE) *.so
