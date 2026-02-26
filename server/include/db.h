#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdint.h>

#include "sensor.h"

int  db_init(const char *path);
int  db_insert_reading(const sensor_channel_t *ch, int64_t timestamp);
void db_close(void);

#endif /* DB_H */
