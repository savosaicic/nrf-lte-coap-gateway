#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SENSOR_MAX_CHANNELS 16
#define SENSOR_NAME_MAX_LEN 16

typedef enum {
  SENSOR_TYPE_FLOAT = 0,
} sensor_type_t;

typedef union {
  float f;
} sensor_value_t;

typedef struct {
  char           name[SENSOR_NAME_MAX_LEN];
  sensor_type_t  type;
  sensor_value_t value;
  bool           has_value;
} sensor_channel_t;

/**
 * @brief One reading captured at snapshot time.
 *
 * Intentionally a plain-data copy with no registry state — safe to
 * hand off to another thread or serialise without holding any lock.
 */
typedef struct {
  char          name[SENSOR_NAME_MAX_LEN];
  sensor_type_t type;
  union {
    float f;
  } value;
} sensor_reading_t;

typedef struct {
  sensor_reading_t readings[SENSOR_MAX_CHANNELS];
  size_t           count;
  int64_t          timestamp_ms;
} sensor_snapshot_t;

sensor_channel_t *sensor_channel_register(const char *name, sensor_type_t type);

int sensor_channel_update_float(sensor_channel_t *ch, float value);

void sensor_snapshot_take(sensor_snapshot_t *snapshot);

#endif /* SENSOR_H */
