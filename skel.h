#ifndef _REDISK_SKEL_
#define _REDISK_SKEL_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct rk_skel_t_ {
  void *opq;
  void (*free)(void *opq);
  bool (*open)(void *opq, const char *path);
  bool (*close)(void *opq);
  /** Redis strings commands */
  char *(*get)(void *opq, const char *kbuf, int ksiz, int *sp);
  bool (*set)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
} rk_skel_t;

typedef void (*rk_skel_init)(rk_skel_t *skel);

#endif
