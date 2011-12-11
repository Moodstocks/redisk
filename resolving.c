#include <stdio.h>
#include <string.h>
#include "resolving.h"

#define ARY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static struct rk_cmd_desc rtable[] = { // TODO handle case
  /** Keys commands */
  {"DEL",rk_do_del,2},
  {"EXISTS",rk_do_exists,2},
  {"TYPE",rk_do_type,2},
  /** Strings commands */
  {"GET",rk_do_get,2},
  {"SET",rk_do_set,3},
  {"SETNX",rk_do_setnx,3},
  {"INCR",rk_do_incr,2},
  {"DECR",rk_do_decr,2},
  {"INCRBY",rk_do_incrby,3},
  {"DECRBY",rk_do_decrby,3},
  {"GETSET",rk_do_getset,3},
  /** Hashes commands */
  {"HGET",rk_do_hget,3},
  {"HSET",rk_do_hset,4},
  {"HDEL",rk_do_hdel,3},
  {"HSETNX",rk_do_hsetnx,4},
  {"HEXISTS",rk_do_hexists,3},
  {"HLEN",rk_do_hlen,2},
  /** Sets commands */
  {"SADD",rk_do_sadd,3},
  {"SREM",rk_do_srem,3},
  {"SCARD",rk_do_scard,2},
  {"SISMEMBER",rk_do_sismember,3},
  {"SMEMBERS",rk_do_smembers,2},
  /** Lists commands */
  {"LLEN",rk_do_llen,2},
  {"LPUSH",rk_do_lpush,3},
  {"RPUSH",rk_do_rpush,3},
  {"LPOP",rk_do_lpop,2},
  {"RPOP",rk_do_rpop,2},
  {"LRANGE",rk_do_lrange,4}
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int i,res;
  for (i=0; i<ARY_SIZE(rtable); ++i) {
    if ( (strlen(rtable[i].name) == args[0]) &&
         !strncmp(argv[0], rtable[i].name, args[0]) ) {
      printf("-> resolved to %s.\n",rtable[i].name);
      if ( rtable[i].arity == argc ) { /* valid command */
        res = rtable[i].fun(skel, argc, argv, args, rsiz, rbuf);
        return res;
      }
      else { /* bad arity */
        printf("-> bad arity (%d, %d)\n",rtable[i].arity,argc);
        char err[256];
        *rsiz = snprintf(err, 256, "-ERR wrong number of arguments for '%.*s' command\r\n", (int)args[0], argv[0]);
        *rbuf = malloc(*rsiz);
        memcpy(*rbuf, err, *rsiz);
        return -1;
      }
    }
  }

  /* if not found... */
  printf("-> command not found\n");
  char err[256];
  *rsiz = snprintf(err, 256, "-ERR unknown command '%.*s'\r\n", (int)args[0], argv[0]);
  *rbuf = malloc(*rsiz);
  memcpy(*rbuf, err, *rsiz);  
  return -1;
}

void fill_err(int *rsiz, char **rbuf) {
  *rsiz = 5;
  *rbuf = malloc(*rsiz);
  memcpy(*rbuf, "$-1\r\n", 5);
}

void fill_ok(int *rsiz, char **rbuf) {
  *rsiz = 5;
  *rbuf = malloc(*rsiz);
  memcpy(*rbuf, "+OK\r\n", 5);
}

void fill_int(int *rsiz, char **rbuf, int n) {
  char nstr[32];
  int nsiz = sprintf(nstr, "%d", n);
  int tsiz = 3 + nsiz;
  *rsiz = tsiz;
  *rbuf = malloc(*rsiz);
  char *wp = *rbuf;
  memcpy(wp, ":", 1);
  wp += 1;
  memcpy(wp, nstr, nsiz);
  wp += nsiz;
  memcpy(wp, "\r\n", 2);
}

#define FILL_POS_INT(n) \
  if (n<0) fill_err(rsiz,rbuf); else fill_int(rsiz,rbuf,n); return 1

void fill_val(int *rsiz, char **rbuf, int ksiz, char *kbuf) {
  char nstr[32];
  int nsiz = sprintf(nstr, "%d", ksiz);
  int tsiz = 5 + nsiz + ksiz;
  *rsiz = tsiz;
  *rbuf = malloc(*rsiz);
  char *wp = *rbuf;
  memcpy(wp, "$", 1);
  wp += 1;
  memcpy(wp, nstr, nsiz);
  wp += nsiz;
  memcpy(wp, "\r\n", 2);
  wp += 2;
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, "\r\n", 2);
  free(kbuf);
}

#define FILL_NNUL_VAL(rs,r) \
  if (r==NULL) fill_err(rsiz,rbuf); else fill_val(rsiz,rbuf,rs,r); return 1

void fill_multi_val(int *rsiz, char **rbuf, rk_listval_t *r) {
  rk_val_t *ary = r->ary;
  int num = r->num;
  int i;
  int tsiz = 0;
  int nsiz;
  char nstr[32];
  nsiz = sprintf(nstr, "%d", num);
  tsiz += 3 + nsiz;
  char **bulks = malloc(num*sizeof(char *));
  for (i = 0; i < num; i++) {
    bulks[i] = malloc(32);
    int siz = sprintf(bulks[i], "%d", ary[i].siz);
    tsiz += 5 + siz + ary[i].siz;
  }
  *rsiz = tsiz;
  *rbuf = malloc(*rsiz);
  char *wp = *rbuf;
  memcpy(wp, "*", 1);
  wp += 1;
  memcpy(wp, nstr, nsiz);
  wp += nsiz;
  memcpy(wp, "\r\n", 2);
  wp += 2;
  for (i = 0; i < num; i++) {
    memcpy(wp, "$", 1);
    wp += 1;
    int siz = strlen(bulks[i]);
    memcpy(wp, bulks[i], siz);
    wp += siz;
    memcpy(wp, "\r\n", 2);
    wp += 2;
    memcpy(wp, ary[i].buf, ary[i].siz);
    wp += ary[i].siz;
    memcpy(wp, "\r\n", 2);
    wp += 2;
    free(bulks[i]);
    free(ary[i].buf);
  }
  free(bulks);
  free(ary);
  free(r);
}

#define FILL_NNUL_MULTI_VAL(r) \
  if (r==NULL) fill_err(rsiz,rbuf); else fill_multi_val(rsiz,rbuf,r); return 1

/** Keys commands */

RK_DO_PROTO(del) {
  int n = skel->del(skel->opq, argv[1], args[1]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(exists) {
  int n = skel->exists(skel->opq, argv[1], args[1]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(type) {
  char *r = skel->type(skel->opq, argv[1], args[1]);
  fill_val(rsiz,rbuf,strlen(r),r);
  return 1;
}

/** Strings commands */

RK_DO_PROTO(get) {
  int rs;
  char *r = skel->get(skel->opq, argv[1], args[1], &rs);
  FILL_NNUL_VAL(rs,r);
}

RK_DO_PROTO(set) {
  skel->set(skel->opq, argv[1], args[1], argv[2], args[2]);
  fill_ok(rsiz,rbuf);
  return 1;
}

RK_DO_PROTO(setnx) {
  int n = skel->setnx(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(incr) {
  int n = skel->incr(skel->opq, argv[1], args[1]);
  fill_int(rsiz,rbuf,n);
  return 1;
}

RK_DO_PROTO(decr) {
  int n = skel->decr(skel->opq, argv[1], args[1]);
  fill_int(rsiz,rbuf,n);
  return 1;
}

RK_DO_PROTO(incrby) {
  char tstr[256];
  strncpy(tstr, argv[2], args[2]);
  tstr[args[2]] = '\0';
  int64_t i = atol(tstr);
  int n = skel->incrby(skel->opq, argv[1], args[1], i);
  fill_int(rsiz,rbuf,n);
  return 1;
}

RK_DO_PROTO(decrby) {
  char tstr[256];
  strncpy(tstr, argv[2], args[2]);
  tstr[args[2]] = '\0';
  int64_t i = atol(tstr);
  int n = skel->decrby(skel->opq, argv[1], args[1], i);
  fill_int(rsiz,rbuf,n);
  return 1;
}

RK_DO_PROTO(getset) {
  int rs;
  char *r = skel->getset(skel->opq, argv[1], args[1], argv[2], args[2], &rs);
  FILL_NNUL_VAL(rs,r);
}

/** Hashes commands */

RK_DO_PROTO(hget) {
  int rs;
  char *r = skel->hget(skel->opq, argv[1], args[1], argv[2], args[2], &rs);
  FILL_NNUL_VAL(rs,r);
}

RK_DO_PROTO(hset) {
  int n = skel->hset(skel->opq, argv[1], args[1], argv[2], args[2], argv[3], args[3]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(hdel) {
  int n = skel->hdel(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(hsetnx) {
  int n = skel->hsetnx(skel->opq, argv[1], args[1], argv[2], args[2], argv[3], args[3]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(hexists) {
  int n = skel->hexists(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(hlen) {
  int n = skel->hlen(skel->opq, argv[1], args[1]);
  FILL_POS_INT(n);
}

/** Sets commands */

RK_DO_PROTO(sadd) {
  int n = skel->sadd(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(srem) {
  int n = skel->srem(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(scard) {
  int n = skel->scard(skel->opq, argv[1], args[1]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(sismember) {
  int n = skel->sismember(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(smembers) {
  rk_listval_t *r = skel->smembers(skel->opq, argv[1], args[1]);
  FILL_NNUL_MULTI_VAL(r);
}

/** Lists commands */

RK_DO_PROTO(llen) {
  int n = skel->llen(skel->opq, argv[1], args[1]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(lpush) {
  int n = skel->lpush(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(rpush) {
  int n = skel->rpush(skel->opq, argv[1], args[1], argv[2], args[2]);
  FILL_POS_INT(n);
}

RK_DO_PROTO(lpop) {
  int rs;
  char *r = skel->lpop(skel->opq, argv[1], args[1], &rs);
  FILL_NNUL_VAL(rs,r);
}

RK_DO_PROTO(rpop) {
  int rs;
  char *r = skel->rpop(skel->opq, argv[1], args[1], &rs);
  FILL_NNUL_VAL(rs,r);
}

RK_DO_PROTO(lrange) {
  char tstr[256];
  int i;
  int range[2];
  for (i = 0; i < 2; i++) {
    strncpy(tstr, argv[i+2], args[i+2]);
    tstr[args[i+2]] = '\0';
    range[i] = atoi(tstr);
  }
  rk_listval_t *r = skel->lrange(skel->opq, argv[1], args[1], range[0], range[1]);
  FILL_NNUL_MULTI_VAL(r);
}
