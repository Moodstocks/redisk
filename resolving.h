#include <stdlib.h>
#include "skel.h"

struct rk_cmd_desc {
  char *name;
  int (*fun)(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
  int arity;
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_get(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_set(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

