#include "sensor.h"

static sensor_registry_t g_registry = {0};

/**
 * @brief Initialize the sensor registry
 * @return Pointer to the initialized registry, or NULL on failure
 */
sensor_registry_t *sensor_reg_init(void)
{
  g_registry.count = 0;
  return &g_registry;
}

/**
 * @brief Close the sensor registry and reset its state
 * @param reg Pointer to the registry to close
 */
void sensor_reg_close(sensor_registry_t *reg)
{
  reg->count = 0;
}
