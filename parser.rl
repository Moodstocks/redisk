#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

static int cs;

%%{
  machine RedisParser;

  action argnum_add_digit { sc->argnum = sc->argnum * 10 + (fc-'0'); }
  action argsize_reset { sc->argsize = 0; }
  action argsize_add_digit { sc->argsize = sc->argsize * 10 + (fc-'0'); }
  action args_init {
    sc->cur_arg = -1;
    sc->args = (char **)malloc(sc->argnum*sizeof(char *));
    sc->arg_sizes = (size_t *)malloc(sc->argnum*sizeof(size_t));
  }
  action arg_init {
    sc->cur_arg++;
    sc->cur_arg_char = 0;
    sc->arg_sizes[sc->cur_arg] = sc->argsize;
    sc->args[sc->cur_arg] = (char *)malloc(sc->argsize);
  }
  action test_arg_len { sc->cur_arg_char < sc->argsize }
  action arg_add_char {
    sc->args[sc->cur_arg][sc->cur_arg_char] = fc;
    sc->cur_arg_char++;
  }

  redis_argnum = '*' ( digit @argnum_add_digit )+ '\r\n';
  redis_argsize = '$' @argsize_reset ( digit @argsize_add_digit )+ '\r\n';
  redis_arg = (any when test_arg_len @arg_add_char)+ '\r\n';
  redis_cmd = redis_argnum @args_init ( redis_argsize @arg_init redis_arg )+;

  main := redis_cmd;
}%%

%% write data;

int redis_parser_init(redis_parser_t *sc) {
  sc->argnum = 0;
  sc->argsize = 0;
  %% write init;
  return 1;
}

int redis_parser_free(redis_parser_t *sc) {
  int i;
  for(i=0; i<sc->argnum; ++i) free(sc->args[i]);
  free(sc->args);
  free(sc->arg_sizes);
  return 1;
}

int redis_parser_exec(redis_parser_t *sc, const char *data, int len) {
  const char *p = data;
  const char *pe = data + len;
  %% write exec;
  if (cs == RedisParser_error) return -1;
  else if (cs >= RedisParser_first_final) return 1;
  else return 0;
}

int redis_parser_finish(redis_parser_t *sc) {
  if (cs == RedisParser_error) return -1;
  else if (cs >= RedisParser_first_final) return 1;
  else return 0;
}

