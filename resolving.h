#include <stdlib.h>
#include "skel.h"

#define RK_DO_PROTO(name) int rk_do_ ## \
  name(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf)

struct rk_cmd_desc {
  char *name;
  int (*fun)(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);
  int arity;
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf);

/** Keys commands */
RK_DO_PROTO(del);
RK_DO_PROTO(exists);
RK_DO_PROTO(type);

/** Strings commands */
RK_DO_PROTO(get);
RK_DO_PROTO(set);
RK_DO_PROTO(setnx);
RK_DO_PROTO(incr);
RK_DO_PROTO(decr);
RK_DO_PROTO(incrby);
RK_DO_PROTO(decrby);
RK_DO_PROTO(getset);

/** Hashes commands */
RK_DO_PROTO(hget);
RK_DO_PROTO(hset);
RK_DO_PROTO(hdel);
RK_DO_PROTO(hsetnx);
RK_DO_PROTO(hexists);
RK_DO_PROTO(hlen);

/** Sets commands */
RK_DO_PROTO(sadd);
RK_DO_PROTO(srem);
RK_DO_PROTO(scard);
RK_DO_PROTO(sismember);

/** Lists commands */
RK_DO_PROTO(llen);
RK_DO_PROTO(lpush);
RK_DO_PROTO(rpush);
RK_DO_PROTO(lpop);
RK_DO_PROTO(rpop);
