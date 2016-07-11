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
    char *buf, *str;
    int num, len;
    Stream_Buf *stream_buf;
    Stream_Buf *update_stream_buf;
    int count;
    EntryPoint *entry_point;
    int result;
    int id;

    count = 0;

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

    database_update_entry(database, entry_point, update_stream_buf, 1);
    assert(result);

    count = database_get_entry_count(database);
    database_add_entry(database, stream_buf);
    database_add_entry(database, stream_buf);
    database_add_entry(database, update_stream_buf);
    assert(database_get_entry_count(database) == (count + 3));

    database_delete_all(database);
    assert(database_get_entry_count(database) == 0);

    destroy_stream_buf(stream_buf);
    destroy_stream_buf(update_stream_buf);

    return 0;
}