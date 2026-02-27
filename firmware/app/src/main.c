#include <zephyr/logging/log.h>

#include "modem.h"
#include "network_events.h"
#include "sensor.h"
#include "coap_backend.h"

LOG_MODULE_REGISTER(nrf_coap_gateway, LOG_LEVEL_DBG);

K_EVENT_DEFINE(network_events);

extern struct k_msgq sensor_msgq;

static const coap_backend_t *coap = &coap_backend_libcoap;

/* Minimal json encoding — TODO: replace with zephyr json library */
static int snapshot_to_json(const sensor_snapshot_t *snapshot, char *buf,
                            size_t buf_len)
{
  int written = 0;
  int ret;

  ret = snprintf(buf + written, buf_len - written,
                 "{\"ts\":%lld,\"readings\":[", snapshot->timestamp_ms);
  if (ret < 0 || (size_t)ret >= buf_len - written) {
    return -ENOMEM;
  }
  written += ret;

  for (size_t i = 0; i < snapshot->count; i++) {
    const sensor_reading_t *r   = &snapshot->readings[i];
    const char             *sep = (i + 1 < snapshot->count) ? "," : "";

    ret = snprintf(buf + written, buf_len - written,
                   "{\"n\":\"%s\",\"t\":%d,\"v\":%.2f}%s", r->name, r->type,
                   (double)r->value.f, sep);
    if (ret < 0 || (size_t)ret >= buf_len - written) {
      return -ENOMEM;
    }
    written += ret;
  }

  ret = snprintf(buf + written, buf_len - written, "]}");
  if (ret < 0 || (size_t)ret >= buf_len - written) {
    return -ENOMEM;
  }
  written += ret;

  return written;
}

#define JSON_BUF_SIZE 1024
int main(void)
{
  int               err;
  sensor_snapshot_t snapshot;
  char              json_buf[JSON_BUF_SIZE];

  /* Connect to lte-m (blocking function) */
  err = modem_configure();
  if (err) {
    LOG_ERR("Modem configuration failed: %d", err);
    return err;
  }

  /* Post an event to threads waiting for network to be configured */
  k_event_post(&network_events, NET_EVENT_LTE_CONNECTED);

  if (coap->init() != 0) {
    LOG_ERR("CoAP backend init failed — thread exiting");
    return -1;
  }

  while (1) {
    k_msgq_get(&sensor_msgq, &snapshot, K_FOREVER);
    int len = snapshot_to_json(&snapshot, json_buf, sizeof(json_buf));
    if (len < 0) {
      LOG_ERR("JSON encoding failed (%d) — dropping snapshot", len);
      continue;
    }

    LOG_DBG("Sending snapshot: %zu readings, %d bytes", snapshot.count, len);
    LOG_DBG("%s", json_buf);

    err = coap->send((const uint8_t *)json_buf, (size_t)len);
    if (err) {
      LOG_ERR("CoAP send failed (%d) — dropping snapshot", err);
      continue;
    }

    err = coap->recv();
    if (err == -ETIMEDOUT) {
      LOG_WRN("CoAP ACK timeout — snapshot may be lost");
    } else if (err) {
      LOG_ERR("CoAP recv error (%d)", err);
    }
  }

  return 0;
}
