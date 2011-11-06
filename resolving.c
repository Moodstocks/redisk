#include <stdio.h>
#include <string.h>
#include "resolving.h"

#define NB_COMMANDS 9

static struct rk_cmd_desc rtable[NB_COMMANDS] = { // TODO handle case
  {"DEL",rk_do_del,2},
  {"EXISTS",rk_do_exists,2},
  {"TYPE",rk_do_type,2},
  {"GET",rk_do_get,2},
  {"SET",rk_do_set,3},
  {"SADD",rk_do_sadd,3},
  {"SREM",rk_do_srem,3},
  {"SCARD",rk_do_scard,2},
  {"SISMEMBER",rk_do_sismember,3}
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

void fill_str(int *rsiz, char **rbuf, int ksiz, char *kbuf) {
  char rstr[256];
  int rstrlen = snprintf(rstr, 255, "%d", ksiz);
  *rsiz = strlen("$\r\n\r\n") + rstrlen + ksiz + 1;
  *rbuf = malloc(*rsiz);
  snprintf(*rbuf, *rsiz, "$%d\r\n%s\r\n", ksiz, kbuf);
}

int rk_do_del(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->del(skel->opq, argv[1], args[1]);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}

int rk_do_exists(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->exists(skel->opq, argv[1], args[1]);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}

int rk_do_type(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  char *r = skel->type(skel->opq, argv[1], args[1]);
  fill_str(rsiz,rbuf,strlen(r),r);
  return 1;
}

int rk_do_get(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int rs;
  char *r = skel->get(skel->opq, argv[1], args[1], &rs);
  if (r == NULL) fill_err(rsiz,rbuf);
  else fill_str(rsiz,rbuf,rs,r);
  return 1;
}

int rk_do_set(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  skel->set(skel->opq, argv[1], args[1], argv[2], args[2]);
  fill_ok(rsiz,rbuf);
  return 1;
}

int rk_do_sadd(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->sadd(skel->opq, argv[1], args[1], argv[2], args[2]);
  printf("SADD %d\n",n);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}

int rk_do_srem(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->srem(skel->opq, argv[1], args[1], argv[2], args[2]);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}

int rk_do_scard(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->scard(skel->opq, argv[1], args[1]);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}

int rk_do_sismember(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int n = skel->sismember(skel->opq, argv[1], args[1], argv[2], args[2]);
  if (n<0) fill_err(rsiz,rbuf);
  else fill_int(rsiz,rbuf,n);
  return 1;
}


