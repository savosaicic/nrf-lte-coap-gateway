#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <coap3/coap.h>

#include "coap_backend.h"

LOG_MODULE_REGISTER(coap_libcoap, LOG_LEVEL_DBG);

#define COAP_SERVER_URI \
	"coap://" CONFIG_COAP_SERVER_HOSTNAME \
	":" STRINGIFY(CONFIG_COAP_SERVER_PORT) "/" CONFIG_COAP_TX_RESOURCE

#define RECV_TIMEOUT_MS 5000

static coap_context_t *g_ctx     = NULL;
static coap_session_t *g_session = NULL;
static coap_uri_t      g_uri;
static coap_address_t  g_dst;

/* Set to 1 by the response handler; reset to 0 before each send */
static volatile int response_received;

static int resolve_address(coap_str_const_t *host, uint16_t port,
                           coap_address_t *dst, int scheme_hint_bits)
{
	coap_addr_info_t *addr_info;

	addr_info = coap_resolve_address_info(host, port, port, port, port,
					      AF_UNSPEC, scheme_hint_bits,
					      COAP_RESOLVE_TYPE_REMOTE);
	if (!addr_info) {
		return 0;
	}

	*dst = addr_info->addr;
	coap_free_address_info(addr_info);
	return 1;
}

static coap_response_t response_handler(coap_session_t   *session,
                                        const coap_pdu_t *sent,
                                        const coap_pdu_t *received,
                                        const coap_mid_t  id)
{
	const uint8_t *data;
	size_t len, offset, total;

	ARG_UNUSED(session);
	ARG_UNUSED(sent);
	ARG_UNUSED(id);

	if (coap_get_data_large(received, &len, &data, &offset, &total)) {
    LOG_INF("Response (%zu/%zu bytes): %*.*s",
		       len + offset, total,
		       (int)len, (int)len, (const char *)data);
		if (len + offset == total) {
			response_received = 1;
		}
	}

	return COAP_RESPONSE_OK;
}

static int libcoap_init()
{
	coap_startup();

  g_ctx = coap_new_context(NULL);
  if (!g_ctx) {
    LOG_ERR("Failed to create CoAP context");
    return -ENOMEM;
  }
  coap_context_set_block_mode(g_ctx, COAP_BLOCK_USE_LIBCOAP);

  /* Parse the server URI */
  if (coap_split_uri((const uint8_t *)COAP_SERVER_URI, strlen(COAP_SERVER_URI),
                     &g_uri) != 0) {
    LOG_ERR("Failed to parse URI: %s", COAP_SERVER_URI);
    return -EINVAL;
  }

  /* Resolve hostname to a coap_address_t */
  if (!resolve_address(&g_uri.host, g_uri.port, &g_dst, 1 << g_uri.scheme)) {
    LOG_ERR("Failed to resolve address for host");
    return -ENOENT;
  }

  /* Open a UDP session toward the server */
  g_session = coap_new_client_session3(g_ctx, NULL, &g_dst, COAP_PROTO_UDP,
                                       NULL, NULL, NULL);
  if (!g_session) {
    LOG_ERR("Failed to create CoAP session");
    return -EIO;
  }

  coap_register_response_handler(g_ctx, response_handler);

  LOG_INF("libcoap3 backend ready");
  return 0;
}

static int libcoap_send(const uint8_t *payload, size_t len)
{
  coap_pdu_t     *pdu     = NULL;
  coap_optlist_t *optlist = NULL;
  int             ret     = -EIO;

  pdu = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST,
                      coap_new_message_id(g_session),
                      coap_session_max_pdu_size(g_session));
  if (!pdu) {
    LOG_ERR("Failed to create PDU");
    goto out;
  }

  if (!coap_uri_into_optlist(&g_uri, &g_dst, &optlist, 1)) {
    LOG_ERR("Failed to build URI options");
    goto out;
  }

  if (coap_add_optlist_pdu(pdu, &optlist) != 1) {
    LOG_ERR("Failed to add options to PDU");
    goto out;
  }

  uint8_t fmt_buf[2];
  size_t  fmt_len = coap_encode_var_safe(fmt_buf, sizeof(fmt_buf),
                                         COAP_MEDIATYPE_APPLICATION_JSON);
  coap_add_option(pdu, COAP_OPTION_CONTENT_FORMAT, fmt_len, fmt_buf);

  coap_add_data(pdu, len, payload);

  if (coap_send(g_session, pdu) == COAP_INVALID_MID) {
    LOG_ERR("coap_send() failed");
    coap_delete_pdu(pdu);
    pdu = NULL;
    goto out;
  }

  pdu = NULL;
  ret = 0;
  LOG_INF("CoAP POST sent (%zu bytes)", len);

out:
  coap_delete_optlist(optlist);
  if (pdu) {
    coap_delete_pdu(pdu);
  }
  return ret;
}

static int libcoap_recv(void)
{
  int64_t deadline;

  response_received = 0;
  deadline = k_uptime_get() + RECV_TIMEOUT_MS;

  while (!response_received && k_uptime_get() < deadline) {
    coap_io_process(g_ctx, 500);
  }

  if (!response_received) {
    LOG_WRN("No response received within %d ms", RECV_TIMEOUT_MS);
  }

  return 0;
}

static void libcoap_cleanup(void)
{
  /* Session is owned by the context; freeing the context is enough */
  coap_free_context(g_ctx);
  g_ctx     = NULL;
  g_session = NULL;
  coap_cleanup();
}

const coap_backend_t coap_backend_libcoap = {
  .init = libcoap_init,
  .send = libcoap_send,
  .recv = libcoap_recv,
  .cleanup = libcoap_cleanup,
};
