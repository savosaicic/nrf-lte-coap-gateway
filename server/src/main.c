#include <stdio.h>

#include "db.h"

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <db-name>\n", argv[0]);
    return -1;
  }

  db_init(argv[1]);

  db_close();
  return 0;
}
