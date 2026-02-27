#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stddef.h>
#include <string.h>

#include "sensor.h"

LOG_MODULE_REGISTER(sensor, LOG_LEVEL_DBG);

static sensor_channel_t g_channels[SENSOR_MAX_CHANNELS];
static size_t           g_channel_count = 0;

K_MUTEX_DEFINE(g_mutex);

sensor_channel_t *sensor_channel_register(const char *name, sensor_type_t type)
{
  sensor_channel_t *ch;

  if (!name || strlen(name) == 0) {
    LOG_ERR("Channel name must not be empty");
    return NULL;
  }

  k_mutex_lock(&g_mutex, K_FOREVER);

  if (g_channel_count >= SENSOR_MAX_CHANNELS) {
    LOG_ERR("Channel registry is full (max %zu)", SENSOR_MAX_CHANNELS);
    k_mutex_unlock(&g_mutex);
    return NULL;
  }

  for (size_t i = 0; i < g_channel_count; i++) {
    if (strcmp(g_channels[i].name, name) == 0) {
      LOG_ERR("Channel `%s` already registered", name);
      k_mutex_unlock(&g_mutex);
      return NULL;
    }
  }

  ch = &g_channels[g_channel_count++];
  memset(ch, 0, sizeof(*ch));
  strncpy(ch->name, name, SENSOR_NAME_MAX_LEN - 1);
  ch->type      = type;
  ch->has_value = false;

  LOG_DBG("Registered channel '%s' (type=%d, slot=%zu)", ch->name, type,
          g_channel_count - 1);
  k_mutex_unlock(&g_mutex);
  return ch;
}

int sensor_channel_update_float(sensor_channel_t *ch, float value)
{
  if (!ch || ch->type != SENSOR_TYPE_FLOAT) {
    return -EINVAL;
  }
  k_mutex_lock(&g_mutex, K_FOREVER);
  ch->value.f   = value;
  ch->has_value = true;
  k_mutex_unlock(&g_mutex);
  return 0;
}

void sensor_snapshot_take(sensor_snapshot_t *snapshot)
{
  if (!snapshot) {
    LOG_ERR("sensor_snapshot_take got a null pointer");
    return;
  }

	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->timestamp_ms = k_uptime_get();

	k_mutex_lock(&g_mutex, K_FOREVER);

	for (size_t i = 0; i < g_channel_count; i++) {
		const sensor_channel_t *ch = &g_channels[i];

		if (!ch->has_value) {
			continue;
		}

		sensor_reading_t *r = &snapshot->readings[snapshot->count++];
		strncpy(r->name, ch->name, SENSOR_NAME_MAX_LEN - 1);
		r->type = ch->type;

		switch (ch->type) {
		case SENSOR_TYPE_FLOAT:
			r->value.f = ch->value.f;
			break;
		}
	}

	k_mutex_unlock(&g_mutex);
}
