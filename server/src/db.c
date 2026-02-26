#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

#include "sensor.h"

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

static int db_channel_lookup(const char *name)
{
  sqlite3_stmt *stmt = NULL;
  int           id = -1;
  int           rc;

  rc = sqlite3_prepare_v2(g_db,
         "SELECT id FROM channels WHERE name = ?",
         -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "db_channel_lookup failed: %s\n",
      sqlite3_errmsg(g_db));
    goto error;
  }

  rc = sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_text failed: %s\n",
      sqlite3_errmsg(g_db));
    goto error;
  }


  if (sqlite3_step(stmt) == SQLITE_ROW) {
    id = sqlite3_column_int(stmt, 0);
  } else {
    fprintf(stderr, "sqlite3_step failed: %s\n",
      sqlite3_errmsg(g_db));
    goto error;
  }

  sqlite3_finalize(stmt);
  return id;

error:
  if (stmt) {
    sqlite3_finalize(stmt);
  }
  return -1;
}

static int db_channel_get_or_create(const char *name, sensor_type_t type)
{
  sqlite3_stmt *stmt = NULL;
  int           id;
  int           rc;

  id = db_channel_lookup(name);
  if (id >= 0) {
    return id;
  }

  rc = sqlite3_prepare_v2(g_db,
    "INSERT INTO channels (name, type) VALUES (?, ?)",
    -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_prepare_v2 failed: %s\n", sqlite3_errmsg(g_db));
    goto error;
  }

  rc = sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_text failed: %s\n", sqlite3_errmsg(g_db));
    goto error;
  }

  rc = sqlite3_bind_int(stmt, 2, (int)type);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_int failed: %s\n", sqlite3_errmsg(g_db));
    goto error;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "sqlite3_step failed: %s\n", sqlite3_errmsg(g_db));
    goto error;
  }

  sqlite3_finalize(stmt);
  return (int)sqlite3_last_insert_rowid(g_db);

error:
  if (stmt) {
    sqlite3_finalize(stmt);
  }
  return -1;
}

void db_close(void)
{
  if (g_db) {
    sqlite3_close(g_db);
    g_db = NULL;
  }
}
