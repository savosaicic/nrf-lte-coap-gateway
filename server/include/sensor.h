#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>

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

#endif /* SENSOR_H */
