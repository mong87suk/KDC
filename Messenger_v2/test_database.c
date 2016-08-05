#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <kdc/DBLinkedList.h>

#include "utils.h"
#include "database.h"
#include "data_file.h"
#include "index_file.h"
#include "entry_point.h"
#include "stream_buf.h"
#include "message_db.h"

static void test_database_match_id(void *data, void *user_data) {
    int id;

    EntryPoint *entry;

    assert(data);
    entry = (EntryPoint *) data;
    id = *((int *) user_data);

    assert(entry_point_get_id(entry) != id); 
}

static void test_database_cmp_str(DataBase *database, int id, char *test_str) {
    char *buf;
    int field_type;

    Stream_Buf *stream_buf;
    EntryPoint *entry_point;
    
    entry_point = database_find_entry_point(database, id);
    assert(entry_point);
    field_type = STRING_FIELD;
    stream_buf = utils_get_data(database, entry_point, 1, &field_type);
    assert(stream_buf);
    buf = stream_buf_get_buf(stream_buf);
    buf += sizeof(LEN_SIZE);
    assert(strcmp(buf, test_str) == 0);

    destroy_stream_buf(stream_buf);
}

int main() {
    char *data_format = "is";
    char *file_name = "test";
    char *buf, *str;
    int num, len;
    int count;
    int result;
    int id;
    int column = 1;
    int id1, id2, id3;
    char *test_str = "aaaa";
    
    Stream_Buf *stream_buf;
    Stream_Buf *update_stream_buf;
    EntryPoint *entry_point;
    DataBase *database;
    DList *entry_list;
    DList *where_list;
    Where *where;

    count = 0;
    where_list = NULL;
    entry_list = NULL;

    database = database_open(file_name, data_format);
    assert(database);
    assert(database_get_field_mask(database) == 0x14);

    stream_buf = new_stream_buf(12);
    assert(stream_buf);
    buf = stream_buf_get_buf(stream_buf);
    assert(buf);
    num = 1;
    len = 4;
    str = "aaaa";
    memcpy(stream_buf_get_available(stream_buf), &num, sizeof(num));
    stream_buf_increase_pos(stream_buf, sizeof(num));

    memcpy(stream_buf_get_available(stream_buf), &len, sizeof(len));
    stream_buf_increase_pos(stream_buf, sizeof(len));

    memcpy(stream_buf_get_available(stream_buf), str, len);
    stream_buf_increase_pos(stream_buf, len);

    count = database_get_entry_count(database);
    id = database_add_entry(database, stream_buf);
    assert(id > 0);

    assert(database_get_entry_count(database) == (count + 1));

    database_delete_entry(database, id);
    assert(database_get_entry_count(database) == count);

    id = database_add_entry(database, stream_buf);
    assert(id > 0);

    entry_point = database_find_entry_point(database, id);
    assert(entry_point);
    assert(entry_point_get_id(entry_point) == id);

    update_stream_buf = new_stream_buf(12);
    assert(stream_buf);
    buf = stream_buf_get_buf(update_stream_buf);
    assert(buf);
    num = 7;
    len = 4;
    str = "bbbb";

    memcpy(stream_buf_get_available(update_stream_buf), &num, sizeof(num));
    stream_buf_increase_pos(update_stream_buf, sizeof(num));

    memcpy(stream_buf_get_available(update_stream_buf), &len, sizeof(len));
    stream_buf_increase_pos(update_stream_buf, sizeof(len));

    memcpy(stream_buf_get_available(update_stream_buf), str, len);
    stream_buf_increase_pos(update_stream_buf, len);

    result = database_update_entry(database, entry_point, update_stream_buf, 1);
    assert(result == TRUE);
    count = database_get_entry_count(database);
    id1 = database_add_entry(database, stream_buf);
    assert(id1);
    test_database_cmp_str(database, id1, test_str);
    id2 = database_add_entry(database, stream_buf);
    assert(id2);
    test_database_cmp_str(database, id2, test_str);
    id3 = database_add_entry(database, update_stream_buf);
    assert(id3);
    test_database_cmp_str(database, id3, str);
    assert(database_get_entry_count(database) == (count + 3));

    database_delete_all(database);
    assert(database_get_entry_count(database) == 0);

    id1 = database_add_entry(database, stream_buf);
    assert(id1);
    id2 = database_add_entry(database, stream_buf);
    assert(id2);
    id3 = database_add_entry(database, update_stream_buf);
    assert(id3);

    where = new_where(column, test_str);
    assert(where);
    where_list = d_list_append(where_list, where);
    assert(where_list);
    entry_list = database_search(database, where_list);
    assert(entry_list);
    assert(d_list_length(entry_list) == 2);

    column = 0;
    num = 1;
    where = new_where(column, &num);
    assert(where);
    where_list = d_list_append(where_list, where);
    assert(where_list);
    assert(d_list_length(where_list) == 2);
    entry_list = database_search(database, where_list);
    assert(entry_list);
    assert(d_list_length(entry_list) == 2);
    d_list_foreach(entry_list, test_database_match_id, &id3);

    destroy_stream_buf(stream_buf);
    destroy_stream_buf(update_stream_buf);

    destory_where_list(where_list);
    database_delete_all(database);
    database_close(database);

    data_format = "ik";
    file_name = "test2";
    database = database_open(file_name, data_format);
    assert(database);
    assert(database_get_field_mask(database) == 0x12);

    stream_buf = new_stream_buf(12);
    assert(stream_buf);
    buf = stream_buf_get_buf(stream_buf);
    assert(buf);
    num = 1;
    len = 4;
    str = "aaaa";
    memcpy(stream_buf_get_available(stream_buf), &num, sizeof(num));
    stream_buf_increase_pos(stream_buf, sizeof(num));

    memcpy(stream_buf_get_available(stream_buf), &len, sizeof(len));
    stream_buf_increase_pos(stream_buf, sizeof(len));

    memcpy(stream_buf_get_available(stream_buf), str, len);
    stream_buf_increase_pos(stream_buf, len);

    count = database_get_entry_count(database);
    id = database_add_entry(database, stream_buf);
    assert(id > 0);

    assert(database_get_entry_count(database) == (count + 1));

    column = 1;
    where = new_where(column, str);
    assert(where);
    where_list = d_list_append(where_list, where);
    assert(where_list);
    entry_list = database_search(database, where_list);
    assert(entry_list);
    assert(d_list_length(entry_list) == 1);

    column = 0;
    num = 1;
    where = new_where(column, &num);
    assert(where);
    where_list = d_list_append(where_list, where);
    assert(where_list);
    assert(d_list_length(where_list) == 2);
    entry_list = database_search(database, where_list);
    assert(entry_list);
    assert(d_list_length(entry_list) == 1);

    return 0;
}