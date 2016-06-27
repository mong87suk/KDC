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
#include "message_db.h"

int main() {
    DataBase *database;
    char *data_format = "is";
    char *file_name = "test";
    char *buf, *str, *cmp_str;
    int num, len;
    Stream_Buf *stream_buf;
    Stream_Buf *entry;
    Stream_Buf *field_buf;
    int count;
    int id;
    EntryPoint *entry_point;
    MessageDB *mesg_db;

    database = new_database(file_name, data_format);
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
    stream_buf_increase_position(stream_buf, sizeof(num));

    memcpy(stream_buf_get_available(stream_buf), &len, sizeof(len));
    stream_buf_increase_position(stream_buf, sizeof(len));

    memcpy(stream_buf_get_available(stream_buf), str, len);

    count = database_get_entry_point_count(database);
    database_add_entry(database, stream_buf);
    assert(database_get_entry_point_count(database) == (count + 1));
    delete_entry(database, 1);
    assert(database_get_entry_point_count(database) == count);

    database_add_entry(database, stream_buf);
    entry = database_get_entry(database, 1);
    buf = stream_buf_get_buf(entry);
    assert(buf);

    num = 0;
    memcpy(&num, buf, sizeof(int));
    assert(num == 1);

    len = 0;
    memcpy(&len, buf + sizeof(int), sizeof(int));
    assert(len == 4);

    cmp_str = (char*) malloc(len);
    memcpy(cmp_str, buf + sizeof(int) + sizeof(int), len);

    assert(strncmp(cmp_str, str, len) == 0);
    delete_entry(database, 1);
    assert(database_get_entry_point_count(database) == 0);

    database_add_entry(database, stream_buf);

    num = 2;
    field_buf = new_stream_buf(4);
    buf = stream_buf_get_buf(field_buf);
    memcpy(buf, &num, sizeof(num));

    database_update_entry(database, 1, 1, field_buf);
    destroy_stream_buf(field_buf);

    len = 4;
    field_buf = new_stream_buf(8);
    buf = stream_buf_get_buf(field_buf);
    memcpy(buf, &len, sizeof(len));

    str = "bbbb";
    memcpy(buf + sizeof(len), str, len);
    database_update_entry(database, 1, 2, field_buf);
    destroy_stream_buf(field_buf);

    entry = database_get_entry(database, 1);
    buf = stream_buf_get_buf(entry);
    assert(buf);

    num = 0;
    memcpy(&num, buf, sizeof(int));
    assert(num == 2);

    len = 0;
    memcpy(&len, buf + sizeof(int), sizeof(int));
    assert(len == 4);

    cmp_str = (char*) malloc(len);
    memcpy(cmp_str, buf + sizeof(int) + sizeof(int), len);

    assert(strncmp(cmp_str, str, len) == 0);

    delete_entry(database, 1);

    id = database_add_entry(database, stream_buf);
    entry_point = database_get_entry_point(database, id);
    assert(id == entry_point_get_id(entry_point));
    delete_entry(database, 1);

    assert(database_convert_data_format_to_field_mask("is") == 0x12);
    database_add_entry(database, stream_buf);
    assert(database_get_entry_point_list(database));
    destroy_database(database);

    return 0;
}
