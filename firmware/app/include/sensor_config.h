#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stddef.h>

#include "data_source.h"

extern data_source_t *g_data_sources[];
extern const size_t   g_data_source_count;

#endif /* !SENSOR_CONFIG_H */
