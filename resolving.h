#include <stdlib.h>

struct rk_cmd_desc {
  char *name;
  int (*fun)(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
  int arity;
};

int resolve(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_get(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_set(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

