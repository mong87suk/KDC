.SUFFIXES: .c.o

DBLINKED_LIST = DBLinkedList

SERVER_MAIN = server_main

CLIENT_MAIN = client_main

TEST_DATABASE = test_database

TEST_TIMER_LOOPER = test_timer_looper

TEST_WATCHER_LOOPER = test_watcher_looper

TEST_ACCOUNT_DB = test_account_db

TEST_MESSAGE_DB = test_message_db

DBLINKED_LIST_SOURCE = ../chapter3/DBLinkedList/DBLinkedList.c

INC = -I../chapter3/DBLinkedList/

DBLINKED_LIST_OBJECT = DBLinkedList.o

SERVER_MAIN_OBJECT = \
					stream_buf.o\
					server_main.o\
					server.o\
					looper.o\
					packet.o\
					converter.o\
					message.o\
					utils.o\
					message_db.o\
					entry_point.o\
					database.o\
					data_file.o\
					index_file.o\
					account.o\
					account_db.o\

CLIENT_MAIN_OBJECT = \
					client_main.o\
					client.o\
					looper.o\
					stream_buf.o\
					packet.o\
					converter.o\
					message.o\
					utils.o\
					account.o\

TEST_DATABASE_OBJECT = \
						test_database.o\
						looper.o\
						utils.o\
						message.o\
						database.o\
						data_file.o\
						index_file.o\
						entry_point.o\
						stream_buf.o\
						message_db.o\

TEST_TIMER_LOOPER_OBJECT = \
						test_timer_looper.o\
						looper.o\

TEST_WATCHER_LOOPER_OBJECT = \
						test_watcher_looper.o\
						looper.o\
						utils.o\
						stream_buf.o\
						message.o\

TEST_ACCOUNT_DB_OBJECT = \
						test_account_db.o\
						database.o\
						index_file.o\
						data_file.o\
						message.o\
						entry_point.o\
						stream_buf.o\
						utils.o\
						account_db.o\
						account.o\

TEST_MESSAGE_DB_OBJET = \
						test_message_db.o\
						database.o\
						index_file.o\
						data_file.o\
						message.o\
						entry_point.o\
						stream_buf.o\
						utils.o\
						message_db.o\

SHARED_LIBRARY = lib$(DBLINKED_LIST).so

CFLAGS +=  -Wall -std=gnu99 -g $(INC)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all: $(SERVER_MAIN) $(CLIENT_MAIN) $(TEST_DATABASE) $(TEST_TIMER_LOOPER) $(TEST_ACCOUNT_DB) $(TEST_MESSAGE_DB) $(TEST_WATCHER_LOOPER)

$(SERVER_MAIN) : $(SERVER_MAIN_OBJECT) $(SHARED_LIBRARY)
	$(CC) -o $(SERVER_MAIN) $(SERVER_MAIN_OBJECT) -L. -l$(DBLINKED_LIST)

$(CLIENT_MAIN) : $(CLIENT_MAIN_OBJECT)
	$(CC) -o $(CLIENT_MAIN) $(CLIENT_MAIN_OBJECT) -L. -l$(DBLINKED_LIST)

$(TEST_DATABASE) : $(TEST_DATABASE_OBJECT)
	$(CC) -o $(TEST_DATABASE) $(TEST_DATABASE_OBJECT) -L. -l$(DBLINKED_LIST)

$(TEST_TIMER_LOOPER) : $(TEST_TIMER_LOOPER_OBJECT)
	$(CC) -o $(TEST_TIMER_LOOPER) $(TEST_TIMER_LOOPER_OBJECT) -L. -l$(DBLINKED_LIST)

$(TEST_WATCHER_LOOPER) : $(TEST_WATCHER_LOOPER_OBJECT)
	$(CC) -o $(TEST_WATCHER_LOOPER) $(TEST_WATCHER_LOOPER_OBJECT) -L. -l$(DBLINKED_LIST)	

$(TEST_ACCOUNT_DB) : $(TEST_ACCOUNT_DB_OBJECT)
	$(CC) -o $(TEST_ACCOUNT_DB) $(TEST_ACCOUNT_DB_OBJECT) -L. -l$(DBLINKED_LIST)

$(TEST_MESSAGE_DB) : $(TEST_MESSAGE_DB_OBJET)
	$(CC) -o $(TEST_MESSAGE_DB) $(TEST_MESSAGE_DB_OBJET) -L. -l$(DBLINKED_LIST)

$(SHARED_LIBRARY):$(DBLINKED_LIST_OBJECT)
	$(CC) -shared -o $@ $^

$(DBLINKED_LIST_OBJECT):
	$(CC) -c -Wall -fpic $(INC) $(DBLINKED_LIST_SOURCE)

clean:
	$(RM) $(SERVER_MAIN) $(CLIENT_MAIN) $(TEST_DATABASE) $(TEST_TIMER_LOOPER) $(TEST_WATCHER_LOOPER) $(TEST_ACCOUNT_DB) $(TEST_MESSAGE_DB) *.o *.so
