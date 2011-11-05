#include <stdio.h>
#include <string.h>
#include "skel.h"
#include "resolving.h"

#define NB_COMMANDS 2

static struct rk_cmd_desc rtable[NB_COMMANDS] = {
  {"get",rk_do_get,2},
  {"set",rk_do_set,3}
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int i,res;
  char rstr[256];
  for (i=0; i<NB_COMMANDS; ++i) {
    if ( (strlen(rtable[i].name) == args[0]) &&
         strncmp(argv[0], rtable[i].name, args[0]) ) {
      if ( rtable[i].arity == argc ) {
        res = rtable[i].fun(skel, argc, argv, args, rsiz, rbuf);
        return res;
      }
      else {
        sprintf(rstr, "-ERR wrong number of arguments for '%s' command\r\n", argv[0]);
        *rsiz = strlen(rstr);
        *rbuf = malloc(*rsiz+1);
        strncpy(*rbuf,rstr,*rsiz);
        *rbuf[*rsiz] = '0';
        return -1;
      }
    }
  }
  /* if not found... */
  sprintf(rstr, "-ERR unknown command '%s'\r\n", argv[0]);
  *rsiz = strlen(rstr);
  *rbuf = malloc(*rsiz+1);
  strncpy(*rbuf,rstr,*rsiz);
  *rbuf[*rsiz] = '0';
  return -1;
}

int rk_do_get(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  *rbuf = skel->get(skel->opq, argv[1], args[1], rsiz);
  return 1;
}

int rk_do_set(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  skel->set(skel->opq, argv[1], args[1], argv[2], args[2]);
  return 1;
}

