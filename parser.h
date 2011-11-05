#include <stdlib.h>

typedef struct redis_parser_s {
	int argnum;
  int argsize;
  int cur_arg;
  int cur_arg_char;
  char **args;
  size_t *arg_sizes;
} redis_parser_t;

int redis_parser_init(redis_parser_t *sc);
int redis_parser_exec(redis_parser_t *sc, const char *data, int len);
int redis_parser_finish(redis_parser_t *sc);
int redis_parser_free(redis_parser_t *sc);
