#ifndef __DATABASE_H__
#define __DATABASE_H__

typedef struct _DataBase DataBase;

DataBase* new_database(char *index_file_name, char *data_file_name);
void destroy_database(DataBase *database);
int add_entry(DataBase *database, char *buf, int field_mask);

#endif
