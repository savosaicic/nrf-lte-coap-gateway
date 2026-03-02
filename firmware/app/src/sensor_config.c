#include <zephyr/sys/util.h>
#include <stddef.h>

#include "data_source.h"

extern data_source_t temperature_sensor_source;

data_source_t *g_data_sources[] = {
  &temperature_sensor_source,
};
const size_t g_data_source_count = ARRAY_SIZE(g_data_sources);
