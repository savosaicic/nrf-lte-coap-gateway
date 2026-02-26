#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

static sqlite3 *g_db = NULL;

static int db_exec(sqlite3 *db, const char *sql)
{
  int   rc;
  char *err_msg;

  rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return -1;
  }
  return 0;
}

int db_init(const char *path)
{
  int rc = sqlite3_open(path, &g_db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to open database '%s': %s\n", path,
            sqlite3_errmsg(g_db));
    sqlite3_close(g_db);
    g_db = NULL;
    return -1;
  }

  const char *sql_channels =
    "CREATE TABLE IF NOT EXISTS channels ("
    "  id    INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name  TEXT    NOT NULL UNIQUE,"
    "  type  INTEGER NOT NULL"
    ");";

  const char *sql_readings =
    "CREATE TABLE IF NOT EXISTS readings ("
    "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  channel_id  INTEGER NOT NULL REFERENCES channels(id),"
    "  timestamp   INTEGER NOT NULL,"
    "  value_float REAL"
    ");";

  const char *sql_index =
    "CREATE INDEX IF NOT EXISTS idx_readings_channel_time"
    "  ON readings(channel_id, timestamp);";

  if (db_exec(g_db, sql_channels) != 0) {
    return -1;
  }
  if (db_exec(g_db, sql_readings) != 0) {
    return -1;
  }
  if (db_exec(g_db, sql_index) != 0) {
    return -1;
  }

  fprintf(stdout, "Database initialized at '%s'\n", path);
  return 0;
}

void db_close(void)
{
  if (g_db) {
    sqlite3_close(g_db);
    g_db = NULL;
  }
}
