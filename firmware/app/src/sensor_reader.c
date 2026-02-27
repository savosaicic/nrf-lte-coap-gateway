#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <stddef.h>

#include "sensor.h"
#include "network_events.h"

LOG_MODULE_REGISTER(sensor_reader, LOG_LEVEL_DBG);

#define STACKSIZE       1024
#define THREAD_PRIORITY 7

K_MSGQ_DEFINE(sensor_msgq, sizeof(sensor_snapshot_t), 4, 1);

float read_temperature() { return (float)sys_rand8_get(); }
float read_humidity() { return (float)sys_rand8_get(); }

static void sensor_reader_thread(void)
{
  sensor_channel_t *ch_temperature =
    sensor_channel_register("temperature", SENSOR_TYPE_FLOAT);
  sensor_channel_t *ch_humidity =
    sensor_channel_register("humidity", SENSOR_TYPE_FLOAT);

  if (!ch_temperature || !ch_humidity) {
    LOG_ERR("Failed to register sensor channels — aborting thread");
    return;
  }

  LOG_INF("Waiting for LTE connection...");
  k_event_wait(&network_events, NET_EVENT_LTE_CONNECTED, false, K_FOREVER);
  LOG_INF("LTE connected — starting sensor reads");

  while (1) {
    sensor_channel_update_float(ch_temperature, read_temperature());
    sensor_channel_update_float(ch_humidity, read_humidity());

    k_msleep(200); /* Simulate reading time */

		sensor_snapshot_t snapshot;
		sensor_snapshot_take(&snapshot);

    if (k_msgq_put(&sensor_msgq, &snapshot, K_NO_WAIT) != 0) {
      LOG_WRN("Queue full — dropping snapshot (%zu readings)", snapshot.count);
		} else {
			LOG_DBG("Enqueued snapshot: %zu readings", snapshot.count);
		}

    k_msleep(CONFIG_SENSOR_READ_INTERVAL_MS);
  }
}

K_THREAD_DEFINE(sensor_reader_id, STACKSIZE, sensor_reader_thread, NULL, NULL,
                NULL, THREAD_PRIORITY, 0, 0);
