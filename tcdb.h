#ifndef _REDISK_TCDB_
#define _REDISK_TCDB_

#include <tchdb.h>
#include <tctdb.h>
#include <tcbdb.h>

#include "skel.h"

typedef struct rk_tcdb_t_ {
  bool open;                /* successful open flag */
  TCHDB *str;               /* main database for strings commands */
  TCTDB *hsh;               /* database for hashes commands */
  TCTDB *set;               /* database for sets commands */
  TCBDB *lst;               /* database for lists commands */
} rk_tcdb_t;

/** Skeleton initialization */
void rk_tcdb_skel_init(rk_skel_t *skel);

rk_tcdb_t *rk_tcdb_new(void);

void rk_tcdb_free(rk_tcdb_t *db);

bool rk_tcdb_open(rk_tcdb_t *db, const char *path);

bool rk_tcdb_close(rk_tcdb_t *db);

/*************************************************
 * KEYS COMMANDS
 *************************************************/
 
/** DEL key */
int rk_tcdb_del(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** EXISTS key */
int rk_tcdb_exists(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** TYPE key */
char *rk_tcdb_type(rk_tcdb_t *db, const char *kbuf, int ksiz);

/*************************************************
 * STRINGS COMMANDS
 *************************************************/

/** GET key */
char *rk_tcdb_get(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp);

/** SET key value */
bool rk_tcdb_set(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz);

/** SETNX key value */
int rk_tcdb_setnx(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz);

/** INCR key */
int64_t rk_tcdb_incr(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** DECR key */
int64_t rk_tcdb_decr(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** INCRBY key increment */
int64_t rk_tcdb_incrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t inc);

/** DECRBY key decrement */
int64_t rk_tcdb_decrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t dec);

/** GETSET key value */
char *rk_tcdb_getset(rk_tcdb_t *db, const char *kbuf, int ksiz,
                     const char *vbuf, int vsiz, int *sp);

/*************************************************
 * HASHES COMMANDS
 *************************************************/
 
/** HGET key field */
char *rk_tcdb_hget(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, int *sp);
                   
/** HSET key field value */
int rk_tcdb_hset(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz, const char *vbuf, int vsiz);
                 
/** HDEL key field */
int rk_tcdb_hdel(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz);
                 
/** HSETNX key field value */
int rk_tcdb_hsetnx(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, const char *vbuf, int vsiz);
                   
/** HEXISTS key field */
int rk_tcdb_hexists(rk_tcdb_t *db, const char *kbuf, int ksiz,
                    const char *fbuf, int fsiz);
                    
/** HLEN key */
int rk_tcdb_hlen(rk_tcdb_t *db, const char *kbuf, int ksiz);

/*************************************************
 * SETS COMMANDS
 *************************************************/
 
/** SADD key member */
int rk_tcdb_sadd(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz);
                 
/** SREM key member */
int rk_tcdb_srem(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz);
                 
/** SCARD key */
int rk_tcdb_scard(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** SISMEMBER key member */
int rk_tcdb_sismember(rk_tcdb_t *db, const char *kbuf, int ksiz,
                      const char *mbuf, int msiz);

/*************************************************
 * LISTS COMMANDS
 *************************************************/

/** LLEN key */
int rk_tcdb_llen(rk_tcdb_t *db, const char *kbuf, int ksiz);

/** LPUSH key value */
int rk_tcdb_lpush(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz);

/** RPUSH key value */
int rk_tcdb_rpush(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz);

/** LPOP key */
char *rk_tcdb_lpop(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp);

/** RPOP key */
char *rk_tcdb_rpop(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp);

/** LRANGE key start stop */
rk_val_t *rk_tcdb_lrange(rk_tcdb_t *db, const char *kbuf, int ksiz,
                         int start, int stop, int *num);

#endif
