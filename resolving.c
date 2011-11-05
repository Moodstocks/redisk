#include <stdio.h>
#include <string.h>
#include "resolving.h"

#define NB_COMMANDS 2

static struct rk_cmd_desc rtable[NB_COMMANDS] = {
  {"get",rk_do_get,2},
  {"set",rk_do_set,3}
};

int resolve(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int i,res;
  for (i=0; i<NB_COMMANDS; ++i) {
    if ( (strlen(rtable[i].name) == args[0]) &&
         strncmp(argv[0], rtable[i].name, args[0]) ) {
      // TODO check arity
      res = rtable[i].fun(argc, argv, args, rsiz, rbuf);
      return res;
    }
  }
  /* if not found... */
  char rstr[256];
  sprintf(rstr, "ERR unknown command '%s'", argv[0]);
  *rsiz = strlen(rstr);
  *rbuf = malloc(*rsiz+1);
  strncpy(*rbuf,rstr,*rsiz);
  *rbuf[*rsiz] = '0';
  return -1;
}

int rk_do_get(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  
  return 1;
}

int rk_do_set(int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  
  return 1;
}

