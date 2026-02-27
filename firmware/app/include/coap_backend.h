#ifndef COAP_BACKEND_H
#define COAP_BACKEND_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief CoAP backend interface
 *
 * @member init     Resolve server, open socket / create session.
 *                  Returns 0 on success, negative errno on failure.
 *
 * @member send     Build and send a CoAP POST.
 *                  Returns 0 on success, negative errno on failure.
 *
 * @member recv     Wait for and process one response.
 *                  Blocks until a response arrives or a timeout expires.
 *                  Returns 0 on success, negative errno on hard error.
 *
 * @member cleanup  Release all resources (socket, session, context, …).
 */
typedef struct coap_backend_s {
  int  (*init)(void);
  int  (*send)(const uint8_t *, size_t);
  int  (*recv)(void);
  void (*cleanup)(void);
} coap_backend_t;

/* libcoap backend            →  coap_libcoap.c */
extern const coap_backend_t coap_backend_libcoap;

#endif /* COAP_BACKEND_H */
