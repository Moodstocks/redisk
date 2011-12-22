CC=gcc
UNAME=$(shell uname)
CFLAGS= -Wall -Werror
_Darwin_ldlags= -lm -lpthread
_Linux_ldflags= -lrt -lm -lpthread
LDFLAGS=$(_$(UNAME)_ldflags)
RAGEL=ragel -G2

CBUILD=$(CC) $(CFLAGS)

REDISK_DEPS= server.c parser.o resolving.o tcdb.o deps/libuv/uv.a deps/tokyocabinet-1.4.47/libtokyocabinet.a

all: redisk parser-test redis-cli-test

deps/libuv/uv.a:
	$(MAKE) -C deps/libuv
	
deps/tokyocabinet-1.4.47/libtokyocabinet.a:
	@/bin/bash -c "pushd deps/tokyocabinet-1.4.47;\
	./configure --disable-shared --disable-zlib \
	--disable-bzip --disable-exlzma --disable-exlzo;\
	popd;"
	$(MAKE) -C deps/tokyocabinet-1.4.47

parser.c: parser.rl
	$(RAGEL) parser.rl

parser.o: parser.c
	$(CBUILD) -c -o parser.o parser.c

resolving.o: resolving.c
	$(CBUILD) -c -o resolving.o resolving.c

tcdb.o: tcdb.c deps/tokyocabinet-1.4.47/libtokyocabinet.a
	$(CBUILD) -c -o tcdb.o tcdb.c -Ideps/tokyocabinet-1.4.47

parser-test: parser.o parser-test.c
	$(CBUILD) -o parser-test parser.o parser-test.c

redisk: $(REDISK_DEPS)
	$(CBUILD) -I. -Ideps/libuv/include -Ideps/tokyocabinet-1.4.47 $(LDFLAGS) \
		-o redisk $(REDISK_DEPS)

redis-cli-test: redis-cli-test.c
	$(CBUILD) -o redis-cli-test redis-cli-test.c

get-deps:
	bash -c " \
		mkdir deps ;\
		pushd deps ;\
			echo 5 ;\
			curl -O http://fallabs.com/tokyocabinet/tokyocabinet-1.4.47.tar.gz ;\
			tar xf tokyocabinet-1.4.47.tar.gz ;\
			rm -f tokyocabinet-1.4.47.tar.gz ;\
			git clone git://github.com/joyent/libuv.git ;\
		popd ;\
	"

clean:
	rm -f redisk
	rm -f parser.o
	rm -f parser.c
	rm -f tcdb.o
	rm -f resolving.o
	rm -f parser-test
	rm -f redis-cli-test

all-clean: clean
	$(MAKE) -C deps/libuv clean
	$(MAKE) -C deps/tokyocabinet-1.4.47 clean

help:
	@echo '[BOOTSTRAP]'
	@echo '  get-deps                         fetch dependencies'
	@echo
	@echo '[BUILD]'
	@echo '  redisk                           build the redisk server (cli)'
	@echo
	@echo '[CLEAN]'
	@echo '  clean                            clean the cli tool and object files'
	@echo '  all-clean                        clean the deps in addition'
