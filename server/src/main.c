#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "db.h"
#include "sensor.h"
#include "coap_server.h"

static volatile bool stop = false;

void handle_sigint(int sig)
{
  (void)sig;
  stop = true;
}

int setup_sig_handler()
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));

  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    return (1);
  }
  return (0);
}

int main(int argc, char **argv)
{
  sensor_registry_t *reg;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <db-name>\n", argv[0]);
    return -1;
  }

  if (setup_sig_handler() != 0) {
    return -1;
  }

  if (db_init(argv[1]) != 0) {
    return -1;
  }

  if (!(reg = sensor_reg_init())) {
    fprintf(stderr, "sensor_reg_init() failed\n");
    return -1;
  }

  if (coap_server_init(COAP_SERVER_PORT) != 0) {
    fprintf(stderr, "coap_server_init() failed\n");
    return -1;
  }

  coap_server_loop(&stop);

  coap_server_cleanup();
  sensor_reg_close(reg);
  db_close();
  return 0;
}
