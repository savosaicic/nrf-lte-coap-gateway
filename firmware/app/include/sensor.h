#ifndef SENSOR_H

#include <stdbool.h>

#define SENSOR_MAX_CHANNELS 16
#define SENSOR_NAME_MAX_LEN 64

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

sensor_channel_t *sensor_channel_register(const char *name, sensor_type_t type);

int sensor_channel_update_float(sensor_channel_t *ch, float value);

#endif /* SENSOR_H */
