#include <stdio.h>
#include <string.h>
#include "parser.h"

#define REDIS_SET_EXAMPLE "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n"

int main(int argc, char *argv[]) {
	redis_parser_t rparser;
	redis_parser_init(&rparser);
  redis_parser_exec(&rparser, REDIS_SET_EXAMPLE, strlen(REDIS_SET_EXAMPLE));
  int finish_state = redis_parser_finish(&rparser);
	if (finish_state <= 0) {
    printf("parsing error (%d)\n", finish_state);
    redis_parser_free(&rparser);
    return 1;
  }
  else {
    printf("parsing ok (%d args)\n", rparser.argnum);
    int i;
    for(i=0; i<rparser.argnum; ++i)
      printf("  %.*s\n", (int)rparser.arg_sizes[i], rparser.args[i]);
    redis_parser_free(&rparser);
    return 0;
  }
}

