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
#include "m_boolean.h"

static void add_entry_for_Type_S(DataBase *database, int count) { 
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
        free(test_buf);
    }
}

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

static void update_entry_for_Type_S(DataBase *database, int id) {
    char *test_buf;
    int size;
    int str_len = 4;
    char *test_str;

    test_str = "8888";

    size = sizeof(int) + str_len;
    test_buf = (char*) malloc(size);
    memset(test_buf, 0, size);
    memcpy(test_buf, &str_len, sizeof(str_len));
    memcpy(test_buf + sizeof(str_len), test_str, str_len);
    LOGD("update_entry \n");
    update_entry(database, id, 1, test_buf);
}

static void show_and_comp_all_entry_for_Type_S(DataBase *database, DList *list, int fd) {
    LOGD("show_all_entry\n");
    int count, i;
    EntryPoint *entry_point;
    Stream_Buf* stream_buf;
    char *buf, *copy_str;
    int len;
    int id;

    count = get_entry_point_count(database);
    for (i = 0; i < count; i++) {
        entry_point = (EntryPoint*) d_list_get_data(list);
        stream_buf = get_value(entry_point, fd);
        buf = get_buf(stream_buf);
        memcpy(&len, buf, sizeof(len));
        buf += sizeof(len);
        id = get_entry_point_id(entry_point);
        comp_entry_for_String(id, len, buf);

        copy_str = (char*) malloc(len + 1);
        memset(copy_str, 0, len + 1);
        memcpy(copy_str, buf, len);
        LOGD("copy_str:%s\n", copy_str);
        list = d_list_next(list);

        destroy_stream_buf(stream_buf);
        free(copy_str);
    }
}

static void show_all_entry(DataBase *database, DList *list, int fd) {
    int count, i;
    EntryPoint *entry_point;
    Stream_Buf* stream_buf;
    char *buf, *copy_str;
    int len;

    count = get_entry_point_count(database);
    for (i = 0; i < count; i++) {
        entry_point = (EntryPoint*) d_list_get_data(list);
        stream_buf = get_value(entry_point, fd);
        buf = get_buf(stream_buf);
        memcpy(&len, buf, sizeof(len));
        buf += sizeof(len);

        copy_str = (char*) malloc(len + 1);
        memset(copy_str, 0, len + 1);
        memcpy(copy_str, buf, len);
        LOGD("copy_str:%s\n", copy_str);

        list = d_list_next(list);
        free(copy_str);
        destroy_stream_buf(stream_buf);
    }
}

static void get_entry_for_type_IS(DataBase *database, int id) {
    Stream_Buf *entry;
    char *buf;
    char *copy_str;
    int len = 4;
    int num;
    int result;

    num = 0;

    entry = get_entry(database, id);
    assert(entry);

    buf = get_buf(entry);
    memcpy(&num, buf, sizeof(num));
    printf("test number:%d", num);
    buf += sizeof(num);

    memcpy(&len, buf, sizeof(len));
    printf(" len:%d", len);
    buf += sizeof(len);

    copy_str = (char*) malloc(len + 1);
    memset(copy_str, 0, len + 1);
    memcpy(copy_str, buf, len);
    printf(" copy_str:%s\n", copy_str);

    result = strncmp(copy_str, "bbbb", 4);
    assert(result == 0);

    destroy_stream_buf(entry);
    free(copy_str);
}

static void add_entry_for_type_IS(DataBase *database) {
    int num;
    int size;
    char *str;
    char *buf;
    int str_len = 4;

    size = sizeof(int) + str_len;
    buf = (char*) malloc(size);
    num = 1;
    memset(buf, 0, size);
    memcpy(buf, &num, sizeof(num));
    str = "aaaa";
    memcpy(buf + sizeof(num), &str_len, sizeof(str_len));
    memcpy(buf + sizeof(num) + sizeof(str_len), str, str_len);
    add_entry(database, buf);

    size = sizeof(int) + str_len;
    num = 2;
    memset(buf, 0, size);
    memcpy(buf, &num, sizeof(num));
    str = "bbbb";
    memcpy(buf + sizeof(num), &str_len, sizeof(str_len));
    memcpy(buf + sizeof(num) + sizeof(str_len), str, str_len);
    add_entry(database, buf);

    size = sizeof(int) + str_len;
    num = 3;
    memset(buf, 0, size);
    memcpy(buf, &num, sizeof(num));
    str = "cccc";
    memcpy(buf + sizeof(num), &str_len, sizeof(str_len));
    memcpy(buf + sizeof(num) + sizeof(str_len), str, str_len);
    add_entry(database, buf);

    free(buf);
}

static void show_all_entry_for_type_IS_format(DataBase *database, DList *list, int fd) {
    LOGD("show_all_entry\n");
    int count, i;
    EntryPoint *entry_point;
    Stream_Buf* stream_buf;
    char *buf, *copy_str;
    int len;
    int num;

    count = get_entry_point_count(database);
    for (i = 0; i < count; i++) {
        entry_point = (EntryPoint*) d_list_get_data(list);
        stream_buf = get_value(entry_point, fd);
        buf = get_buf(stream_buf);
        memcpy(&num, buf, sizeof(num));
        printf("test number:%d", num);
        buf += sizeof(num);

        memcpy(&len, buf, sizeof(len));
        printf(" len:%d", len);
        buf += sizeof(len);

        copy_str = (char*) malloc(len + 1);
        memset(copy_str, 0, len + 1); 
        memcpy(copy_str, buf, len);
        printf(" copy_str:%s\n", copy_str);

        list = d_list_next(list);
        destroy_stream_buf(stream_buf);
        free(copy_str);
    }
}

int main() {
    // s is String
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

    //Add entry for Type: s
    LOGD("\n\nAdd entry : 10\n\n");
    add_entry_for_Type_S(database, 10);
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
    show_and_comp_all_entry_for_Type_S(database, list, data_file_fd);

    //Delete entry
    LOGD("\n\nDelete entry id: 3\n\n");
    count = get_entry_point_count(database);
    LOGD("before delete entry count:%d\n", count);
    delete_entry(database, 3);
    count = get_entry_point_count(database);
    LOGD("after delete entry count:%d\n", count);
    assert(delete_entry(database, 3) == FALSE);
    list = get_entry_point_list(database);
    show_all_entry_id(database, list);
    show_all_entry(database, list, data_file_fd);

    //Update entry S
    // S is string
    LOGD("\n\nUpdate entry id:1 String:8888\n\n");
    update_entry_for_Type_S(database, 1);
    show_all_entry(database, list, data_file_fd);

    //Create database
    database = new_database("is_test", "is");

    //Add entry for Type "IS"
    // I : integer, S: String
    add_entry_for_type_IS(database);
    list = get_entry_point_list(database);
    count = get_entry_point_count(database);
    LOGD("entry_count:%d\n", count);
    data_file = new_data_file("is_test");
    assert(data_file);
    offset = get_data_file_offset(data_file);
    LOGD("data_file_offset:%d\n", offset);
    data_file_fd = get_data_file_fd(data_file);
    show_all_entry_for_type_IS_format(database, list, data_file_fd);

    //Get entry of id :2
    LOGD("\n\nGet entry id: 2\n\n");
    get_entry_for_type_IS(database, 2);
    return 0;
}
