#include <stdlib.h>
#include "skel.h"

struct rk_cmd_desc {
  char *name;
  int (*fun)(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
  int arity;
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

int rk_do_del(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_exists(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_type(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

int rk_do_get(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_set(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

int rk_do_sadd(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_srem(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_scard(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
int rk_do_sismember(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

