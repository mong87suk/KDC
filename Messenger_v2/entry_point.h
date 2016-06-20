#ifndef __ENTRY_POINT_H__
#define __ENTRY_POINT_H__

typedef struct _Entry_Point Entry_Point;

Entry_Point* new_entry_point();
void destroy_entry_point(Entry_Point *entry_point);
int get_entry_point_size();
int set_offset(Entry_Point *entry_point, int offset);
int set_id(Entry_Point *entry_point, int id);
int set_value(Entry_Point *entry_point, int fd, int field_mask, char *buf);

#endif
