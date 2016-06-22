#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "database.h"
#include "data_file.h"
#include "index_file.h"
#include "entry_point.h"
#include "DBLinkedList.h"
#include "stream_buf.h"

// Create test entry. entry is 8 byte. There is only string.
static void add_entry_for_string(DataBase *database, int count) { 
    char *test_buf;
    char test_str[4];
    int size;
    int str_len = 4;
    size = sizeof(int) + str_len;

    int i, j;
    for (i = 1; i <= count; i++) {

        for (j = 0; j < 4; j++) {
            test_str[j] = 48 + (i % 10);
        }

        test_buf = (char*) malloc(size);
        memset(test_buf, 0, size);
        memcpy(test_buf, &str_len, sizeof(str_len));
        memcpy(test_buf + sizeof(str_len), test_str, str_len);
        add_entry(database, test_buf);
        LOGD("test\n");
        free(test_buf);
    }
}

//Show all entry id
static void show_all_entry_id(DataBase *database, DList *list) {
    int count, i;
    int id;
    EntryPoint *entry_point;

    count = get_entry_point_count(database);
    for (i = 0; i < count; i++) {
        entry_point = (EntryPoint*) d_list_get_data(list);
        id = get_entry_point_id(entry_point);
        list = d_list_next(list);
        LOGD("entry id:%d\n", id);
    }
}

//Comp entry value
static void comp_entry_for_String(int i, int len, char *buf) {
    int index;
    int value;
    int result;
    char comp_buf[4];

    value = i % 10;

    assert(len == 4);
    for (index = 0;  index < 4; index++) {
        comp_buf[index] = 48 + value;
    }

    result = strncmp(buf, comp_buf, 4);
    assert(result == 0);
}

//Show all entry
void show_all_entry(DataBase *database, DList *list) {
    LOGD("show_all_entry\n");
    int count, i;
    EntryPoint *entry_point;
    Stream_Buf* stream_buf;
    char *buf, *copy_buf;
    int len;
    int id;

    count = get_entry_point_count(database);
    for (i = 0; i < count; i++) {
        entry_point = (EntryPoint*) d_list_get_data(list);
        stream_buf = get_value(entry_point);
        buf = get_buf(stream_buf);
        memcpy(&len, buf, sizeof(len));
        buf += sizeof(len);
        id = get_entry_point_id(entry_point);
        comp_entry_for_String(id, len, buf);

        copy_buf = (char*) malloc(len + 1);
        memset(copy_buf, 0, len + 1);
        memcpy(copy_buf, buf, len);
        LOGD("copy_buf:%s\n", copy_buf);

        list = d_list_next(list);
        free(copy_buf);
    }
}

int main() {
    char *data_format = "s";
    char *file_name = "test";
    DataBase *database;
    DataFile *data_file;
    DList *list;
    int data_file_fd;
    int count;
    int offset;

    LOGD("\n\ndata_format:%s\n\n", data_format);

    //Create database
    database = new_database(file_name, data_format);
    assert(database);

    //Count
    LOGD("\n\nentry count\n\n");
    count = get_entry_point_count(database);
    LOGD("entry_count:%d\n", count);

    //Add entry
    LOGD("\n\nAdd entry : 10\n\n");
    add_entry_for_string(database, 10);
    count = get_entry_point_count(database);
    LOGD("entry_coutn:%d\n", count);

    //Create Data file
    LOGD("\n\nDataFile offset\n\n");
    data_file = new_data_file(file_name);
    assert(data_file);
    offset = get_data_file_offset(data_file);
    LOGD("data_file_offset:%d\n", offset);
    data_file_fd = get_data_file_fd(data_file);
    assert(data_file_fd > 0);

    //Get Entry Point list
    LOGD("\n\nShow Entry id and Entry String value\n\n");
    list = get_entry_point_list(database);
    show_all_entry_id(database, list);
    show_all_entry(database, list);

    //Delete entry
    LOGD("\n\nDelete entry id: 3\n\n");
    count = get_entry_point_count(database);
    delete_entry(database, 3);
    assert(get_entry_point_count(database) == (count - 1));
    list = get_entry_point_list(database);
    show_all_entry_id(database, list);
    show_all_entry(database, list);

    //Update entry
    LOGD("\n\nUpdate entry id:1 String:8888\n\n");
    int id = 3;
    int colum = 1;
    
    update_entry(database, id, colum, 

    return 0;
}
