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
  /** Keys commands */
  int (*del)(void *opq, const char *kbuf, int ksiz);
  int (*exists)(void *opq, const char *kbuf, int ksiz);
  char *(*type)(void *opq, const char *kbuf, int ksiz);
  /** Strings commands */
  char *(*get)(void *opq, const char *kbuf, int ksiz, int *sp);
  bool (*set)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
  int (*setnx)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
  int64_t (*incr)(void *opq, const char *kbuf, int ksiz);
  int64_t (*decr)(void *opq, const char *kbuf, int ksiz);
  int64_t (*incrby)(void *opq, const char *kbuf, int ksiz, int64_t inc);
  int64_t (*decrby)(void *opq, const char *kbuf, int ksiz, int dec);
  char *(*getset)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz, int *sp);
  /** Hashes commands */
  char *(*hget)(void *opq, const char *kbuf, int ksiz, const char *fbuf, int fsiz, int *sp);
  int (*hset)(void *opq, const char *kbuf, int ksiz, const char *fbuf, int fsiz, const char *vbuf, int vsiz);
  int (*hdel)(void *opq, const char *kbuf, int ksiz, const char *fbuf, int fsiz);
  int (*hsetnx)(void *opq, const char *kbuf, int ksiz, const char *fbuf, int fsiz, const char *vbuf, int vsiz);
  int (*hexists)(void *opq, const char *kbuf, int ksiz, const char *fbuf, int fsiz);
  int (*hlen)(void *opq, const char *kbuf, int ksiz);
  /** Sets commands */
  int (*sadd)(void *opq, const char *kbuf, int ksiz, const char *mbuf, int msiz);
  int (*srem)(void *opq, const char *kbuf, int ksiz, const char *mbuf, int msiz);
  int (*scard)(void *opq, const char *kbuf, int ksiz);
  int (*sismember)(void *opq, const char *kbuf, int ksiz, const char *mbuf, int msiz);
  /** Lists commands */
  int (*llen)(void *opq, const char *kbuf, int ksiz);
  int (*lpush)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
  int (*rpush)(void *opq, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
  char *(*lpop)(void *opq, const char *kbuf, int ksiz, int *sp);
  char *(*rpop)(void *opq, const char *kbuf, int ksiz, int *sp);
} rk_skel_t;

typedef void (*rk_skel_init)(rk_skel_t *skel);

#endif
