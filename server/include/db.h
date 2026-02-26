#ifndef DB_H
#define DB_H

#include <sqlite3.h>

int  db_init(const char *path);
void db_close(void);

#endif /* DB_H */
