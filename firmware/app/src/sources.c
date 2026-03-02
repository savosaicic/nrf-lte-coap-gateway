#include <stddef.h>

#include "sensor_config.h"

int sources_init_all(void)
{
  for (size_t i = 0; i < g_data_source_count; i++) {
    int err = g_data_sources[i]->init ? g_data_sources[i]->init() : 0;
    if (err) {
      return err;
    }
  }
  return 0;
}

int sources_read_all(void)
{
  for (size_t i = 0; i < g_data_source_count; i++) {
    if (g_data_sources[i]->read) {
      (void)g_data_sources[i]->read(); /* sample failures are not fatal */
    }
  }
  return 0;
}
