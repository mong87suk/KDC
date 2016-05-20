.SUFFIXES: .c.o

DBLINKED_LIST = DBLinkedList

SERVER_MAIN = server_main

CLIENT_MAIN = client_main

DBLINKED_LIST_SOURCE = ../chapter3/DBLinkedList/DBLinkedList.c

INC = -I../chapter3/DBLinkedList/

DBLINKED_LIST_OBJECT = DBLinkedList.o

SERVER_MAIN_OBJECT = \
			server_main.o\
			server.o\
			mesg_file.o\
			mesg_file_db.o\
			looper.o\
			stream_buf.o\
			packet.o\

CLIENT_MAIN_OBJECT = \
			client_main.o\
			client.o\
			looper.o\
			stream_buf.o\
			packet.o\

SHARED_LIBRARY = lib$(DBLINKED_LIST).so

CFLAGS += -Wall -g $(INC)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(SERVER_MAIN) $(CLIENT_MAIN)

$(SERVER_MAIN) : $(SERVER_MAIN_OBJECT) $(SHARED_LIBRARY)
	$(CC) -o $(SERVER_MAIN) $(SERVER_MAIN_OBJECT) -L. -l$(DBLINKED_LIST)

$(CLIENT_MAIN) : $(CLIENT_MAIN_OBJECT)
	$(CC) -o $(CLIENT_MAIN) $(CLIENT_MAIN_OBJECT) -L. -l$(DBLINKED_LIST)

$(SHARED_LIBRARY):$(DBLINKED_LIST_OBJECT)
	$(CC) -shared -o $@ $^

$(DBLINKED_LIST_OBJECT):
	$(CC) -c -Wall -fpic $(INC) $(DBLINKED_LIST_SOURCE)

clean:
	$(RM) $(SERVER_MAIN) $(CLIENT_MAIN) *.o *.so
