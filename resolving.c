#include <stdio.h>
#include <string.h>
#include "resolving.h"

#define NB_COMMANDS 26

static struct rk_cmd_desc rtable[NB_COMMANDS] = { // TODO handle case
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
  /** Lists commands */
  {"LLEN",rk_do_llen,2},
  {"LPUSH",rk_do_lpush,3},
  {"RPUSH",rk_do_rpush,3},
  {"LPOP",rk_do_lpop,2},
  {"RPOP",rk_do_rpop,2}
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int i,res;
  for (i=0; i<NB_COMMANDS; ++i) {
    if ( (strlen(rtable[i].name) == args[0]) &&
         !strncmp(argv[0], rtable[i].name, args[0]) ) {
      printf("-> resolved to %s.\n",rtable[i].name);
      if ( rtable[i].arity == argc ) { /* valid command */
        res = rtable[i].fun(skel, argc, argv, args, rsiz, rbuf);
        return res;
      }
      else { /* bad arity */
        printf("-> bad arity (%d, %d)\n",rtable[i].arity,argc);
        *rsiz = strlen("-ERR wrong number of arguments for '' command\r\n") + args[0] + 1;
        *rbuf = malloc(*rsiz);
        snprintf(*rbuf, *rsiz, "-ERR wrong number of arguments for '%.*s' command\r\n", (int)args[0], argv[0]);
        return -1;
      }
    }
  }

  /* if not found... */
  printf("-> command not found\n");
  *rsiz = strlen("-ERR unknown command ''\r\n") + args[0] + 1;
  *rbuf = malloc(*rsiz);
  snprintf(*rbuf, *rsiz, "-ERR unknown command '%.*s'\r\n", (int)args[0], argv[0]);
  return -1;
}

void fill_err(int *rsiz, char **rbuf) {
  *rbuf = "$-1\r\n";
  *rsiz = strlen(*rbuf)+1;
}

void fill_ok(int *rsiz, char **rbuf) {
  *rbuf = "+OK\r\n";
  *rsiz = strlen(*rbuf)+1;
}

void fill_int(int *rsiz, char **rbuf, int n) {
  char rstr[256];
  int rstrlen = snprintf(rstr, 255, "%d", n);
  *rsiz = strlen(":\r\n") + rstrlen + 1;
  *rbuf = malloc(*rsiz);
  snprintf(*rbuf, *rsiz, ":%d\r\n", n);
}

#define FILL_POS_INT(n) \
  if (n<0) fill_err(rsiz,rbuf); else fill_int(rsiz,rbuf,n); return 1

void fill_str(int *rsiz, char **rbuf, int ksiz, char *kbuf) {
  char rstr[256];
  int rstrlen = snprintf(rstr, 255, "%d", ksiz);
  *rsiz = strlen("$\r\n\r\n") + rstrlen + ksiz + 1;
  *rbuf = malloc(*rsiz);
  snprintf(*rbuf, *rsiz, "$%d\r\n%s\r\n", ksiz, kbuf);
}

#define FILL_NNUL_STR(rs,r) \
  if (r==NULL) fill_err(rsiz,rbuf); else fill_str(rsiz,rbuf,rs,r); return 1

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
  fill_str(rsiz,rbuf,strlen(r),r);
  return 1;
}

/** Strings commands */

RK_DO_PROTO(get) {
  int rs;
  char *r = skel->get(skel->opq, argv[1], args[1], &rs);
  FILL_NNUL_STR(rs,r);
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
  FILL_NNUL_STR(rs,r);
}

/** Hashes commands */

RK_DO_PROTO(hget) {
  int rs;
  char *r = skel->hget(skel->opq, argv[1], args[1], argv[2], args[2], &rs);
  FILL_NNUL_STR(rs,r);
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
  FILL_NNUL_STR(rs,r);
}

RK_DO_PROTO(rpop) {
  int rs;
  char *r = skel->rpop(skel->opq, argv[1], args[1], &rs);
  FILL_NNUL_STR(rs,r);
}
