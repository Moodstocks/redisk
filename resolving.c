#include <stdio.h>
#include <string.h>
#include "resolving.h"

#define NB_COMMANDS 2

static struct rk_cmd_desc rtable[NB_COMMANDS] = { // TODO handle case
  {"GET",rk_do_get,2},
  {"SET",rk_do_set,3}
};

int resolve(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int i,res;
  for (i=0; i<NB_COMMANDS; ++i) {
    printf("command %d\n",i);
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

int rk_do_get(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  int rs;
  char rstr[256];
  char *r = skel->get(skel->opq, argv[1], args[1], &rs);
  if (r == NULL) {
    *rbuf = "$-1\r\n";
    *rsiz = strlen(*rbuf)+1;
  }
  else {
    int rstrlen = snprintf(rstr, 255, "%d", rs);
    *rsiz = strlen("$\r\n\r\n") + rstrlen + rs + 1;
    *rbuf = malloc(*rsiz);
    snprintf(*rbuf, *rsiz-1, "$%d\r\n%s\r\n", rs, r);
  }
  return 1;
}

int rk_do_set(rk_skel_t *skel, int argc, char *argv[], size_t *args, int *rsiz, char **rbuf) {
  skel->set(skel->opq, argv[1], args[1], argv[2], args[2]);
  *rbuf = "+OK\r\n";
  *rsiz = strlen(*rbuf)+1;
  return 1;
}

