#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include "resolving.h"
#include "parser.h"
#include "skel.h"
#include "tcdb.h"
#include "deps/libuv/include/uv.h"

#define OK_RESPONSE "+OK\r\n"

static uv_loop_t *uv_loop;
static uv_tcp_t server;
static redis_parser_t rparser;
static rk_skel_t *skel;
rk_skel_init g_skel_init = rk_tcdb_skel_init;

typedef struct {
  uv_tcp_t handle;
  uv_write_t write_req;
  uv_buf_t buf;
} client_t;

void on_close(uv_handle_t *handle);
void after_write(uv_write_t *req, int status);
void on_read(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf);
void on_connect(uv_stream_t* server_handle, int status);
int on_exit(void);
void register_sig_handler(int signal, void (*handler)(int));
void signal_exit(int signal);

void on_close(uv_handle_t *handle) {
  printf("connection closed\n");
}

uv_buf_t on_alloc(uv_handle_t *client, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

void after_write(uv_write_t *req, int status) {
  if (status) {
    uv_err_t err = uv_last_error(uv_loop);
    fprintf(stderr, "write error: %s\n", uv_strerror(err));
    exit(1);
  }
  // TODO free(buf.base);
  //uv_close((uv_handle_t*)req->handle, on_close);
}

void on_read(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf) {
  printf("=> reading...\n");
  client_t *client = (client_t *)tcp->data;

  /* PARSE */
  if (nread >= 0) {
  	redis_parser_init(&rparser);
    redis_parser_exec(&rparser, buf.base, nread);
    int finish_state = redis_parser_finish(&rparser);
  	if (finish_state <= 0) {
      printf("parsing error (%d)\ninput was:\n", finish_state);
      printf("%s\n", buf.base);
      redis_parser_free(&rparser);
      exit(1); // be violent...
    }
    printf("parsing ok (%d args)\n", rparser.argnum);
    int i;
    for(i=0; i<rparser.argnum; ++i)
      printf("  %.*s\n", (int)rparser.arg_sizes[i], rparser.args[i]);

    int rsiz;
    char *rbuf;
    resolve(skel, rparser.argnum, rparser.args, rparser.arg_sizes, &rsiz, &rbuf);

    client->buf.base = rbuf;
    client->buf.len = rsiz;
    redis_parser_free(&rparser);
    uv_write( &client->write_req, (uv_stream_t *)&client->handle,
              &client->buf, 1, after_write );
  }
  else {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF)
      fprintf(stderr, "read error: %s\n", uv_strerror(err));
    else {
      printf("=> received EOF\n");
      uv_close((uv_handle_t*)((&client->write_req)->handle), on_close);
    }
  }

  free(buf.base);
}

void on_connect(uv_stream_t* server_handle, int status) {
  assert((uv_tcp_t *)server_handle == &server);
  client_t* client = malloc(sizeof(client_t));
  printf("=> new connection\n");
  uv_tcp_init(uv_loop, &client->handle);
  client->handle.data = client;
  uv_accept(server_handle, (uv_stream_t*)&client->handle);
  printf("=> before read_start\n");
  uv_read_start((uv_stream_t *)&client->handle, on_alloc, on_read);
}

int on_exit(void) {
  int rv = 0;
  if (skel != NULL) {
    rv = skel->close(skel->opq) ? 0 : 1;
    skel->free(skel->opq);
    free(skel);
    skel = NULL;
  }
  return rv;
}

void register_sig_handler(int signal, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(signal, &sa, NULL);
}

void signal_exit(int signal) {
  exit(on_exit());
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "USAGE: redisk PORT\n");
    exit(1);
  }
  int port = atoi(argv[1]);
  register_sig_handler(SIGINT, signal_exit);
  register_sig_handler(SIGTERM, signal_exit);

  skel = malloc(sizeof(*skel));
  g_skel_init(skel);
  if (!skel->open(skel->opq, "foo.db"))
    fprintf(stderr, "skel open error\n");
  uv_loop = uv_default_loop();
  uv_tcp_init(uv_loop, &server);
  struct sockaddr_in address = uv_ip4_addr("0.0.0.0", port);
  uv_tcp_bind(&server, address);
  uv_listen((uv_stream_t *)&server, 128, on_connect);
  printf("listening on port %d\n",port);
  uv_run(uv_loop);
  return 0;
}

