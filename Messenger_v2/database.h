#ifndef __DATABASE_H__
#define __DATABASE_H__

#define INTEGER_FIELD (int) 0x10000000
#define STRING_FIELD  (int) 0x20000000

typedef struct _DataBase DataBase;

DataBase* new_database(char *file_name, char *filed_mask);
void destroy_database(DataBase *database);
int add_entry(DataBase *database, char *buf);

#endif
