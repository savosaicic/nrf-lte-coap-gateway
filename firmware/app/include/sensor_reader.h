#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <zephyr/kernel.h>

/** 
 * Message queue through which the sensor reader thread delivers
 * sensor_snapshot_t objects.
 */
extern struct k_msgq sensor_msgq;

#endif /* SENSOR_READER_H */
