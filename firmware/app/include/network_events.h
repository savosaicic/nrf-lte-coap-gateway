#ifndef NETWORK_EVENTS_H
#define NETWORK_EVENTS_H

#include <zephyr/kernel.h>

#define NET_EVENT_LTE_CONNECTED BIT(0)

extern struct k_event network_events;

#endif /* NETWORK_EVENTS_H */
