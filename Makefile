CC=gcc
UNAME=$(shell uname)
CFLAGS= -Wall -Werror
_Darwin_ldlags= -lm -lpthread
_Linux_ldflags= -lrt -lm -lpthread
LDFLAGS=$(_$(UNAME)_ldflags)
RAGEL=ragel -G2

CBUILD=$(CC) $(CFLAGS)

all: redisk parser-test redis-cli-test

deps/libuv/uv.a:
	$(MAKE) -C deps/libuv
	
deps/tokyocabinet-1.4.47/libtokyocabinet.a:
	@/bin/bash -c "pushd deps/tokyocabinet-1.4.47;\
	./configure --disable-shared;\
	popd;"
	$(MAKE) -C deps/tokyocabinet-1.4.47

parser.c: parser.rl
	$(RAGEL) parser.rl

parser.o: parser.c
	$(CBUILD) -c -o parser.o parser.c

parser-test: parser.o parser-test.c
	$(CBUILD) -o parser-test parser.o parser-test.c

redisk: server.c parser.o deps/libuv/uv.a
	$(CBUILD) -I. -Ideps/libuv/include $(LDFLAGS) \
		-o redisk server.c parser.o deps/libuv/uv.a

redis-cli-test: redis-cli-test.c
	$(CBUILD) -o redis-cli-test redis-cli-test.c

clean:
	rm -f redisk
	rm -f parser.o
	rm -f parser.c
	rm -f parser-test
	rm -f redis-cli-test

all-clean: clean
	$(MAKE) -C deps/libuv clean
	$(MAKE) -C deps/tokyocabinet-1.4.47 clean
