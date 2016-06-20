#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "looper.h"
#include "utils.h"
#include "socket.h"
#include "database.h"
#include "data_file.h"
#include "index_file.h"
#include "entry_point.h"
#include "m_boolean.h"

int main() {
    char *data_format = "s";
    char *test_str = "test";
    char *test_buf;
    char *file_name = "test";
    DataBase *database;
    Data_File *data_file;
    int size;
    int str_len;
    int data_file_fd;

    // Create test entry. entry is 8 byte.
    str_len = 4;
    size = sizeof(int) + str_len;
    test_buf = (char*) malloc(size);
    memcpy(test_buf, &str_len, sizeof(str_len));
    memcpy(test_buf + sizeof(str_len), test_str, str_len);

    //Create database
    database = new_database(file_name, data_format);
    assert(database);

    //Destry database
    destroy_database(database);

    //Add entry
    database = new_database(file_name, data_format);
    add_entry(database, test_buf);
    destroy_database(database);

    //Create Data file
    data_file = new_data_file(file_name);
    assert(data_file);
    assert(get_data_file_offset(data_file) == 0);
    data_file_fd = get_data_file_fd(data_file);
    assert(data_file_fd > 0);

    return 0;
}
