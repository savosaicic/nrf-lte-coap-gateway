#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

/* COAP_RESOURCE_CHECK_TIME is 1 second in libcoap — how often
   the library checks for observable resource updates and retransmits */
#define COAP_SERVER_TIMEOUT_MS (COAP_RESOURCE_CHECK_TIME * 1000)

int  coap_server_init(uint16_t port);
void coap_server_cleanup(void);
void coap_server_loop(volatile bool *stop);

#endif /* !COAP_SERVER_H */
