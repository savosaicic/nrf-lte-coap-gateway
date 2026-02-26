#ifndef SNAPSHOT_PARSER_H
#define SNAPSHOT_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include "sensor.h"

typedef sensor_channel_t parsed_reading_t;

typedef struct {
  parsed_reading_t readings[SENSOR_MAX_CHANNELS];
  size_t           count;
} parsed_snapshot_t;

int parse_snapshot_json(const char *buf, size_t len, parsed_snapshot_t *out);

#endif /* SNAPSHOT_PARSER_H */
