#ifndef __ENTRY_POINT_H__
#define __ENTRY_POINT_H__

typedef struct _EntryPoint EntryPoint;

EntryPoint* new_entry_point(int id, int fd, int offset, int field_mask);
void destroy_entry_point(EntryPoint *entry_point);
int get_entry_point_size();
int set_value(EntryPoint *entry_point, char *buf);

#endif
