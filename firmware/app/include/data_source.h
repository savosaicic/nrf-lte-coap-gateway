#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

typedef struct {
  const char *name; /* eg: "lsm303agr" */

  /* Allocate channels and configure hardware
   * return - on success, negative errno on failure
   */
  int (*init)(void);

  /* Called by sensor_reader thread
   * Driver reads hardware and calls sensor_channel_update_*() internally
   * return 0 on success, negative errno on failure
   */
  int (*read)(void);

} data_source_t;

#endif /* !DATA_SOURCE_H */
