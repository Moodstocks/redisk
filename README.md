                  _ _     _    
                 | (_)   | |   
     _ __ ___  __| |_ ___| | __
    | '__/ _ \/ _` | / __| |/ /
    | | |  __/ (_| | \__ \   < 
    |_|  \___|\__,_|_|___/_|\_\
                           

## Warning

DO NOT USE IN PRODUCTION. I REPEAT: DO NOT USE IN PRODUCTION (OR FOR ANYTHING IMPORTANT).

This is a hack developed for a hackathon. We intend to continue working on it and release it someday but currently the project is definitely not in a usable state, except for demo / test purpose.

## What it is?

redisk is an embeddable, portable (hopefully), disk-persistent [Redis](http://redis.io/) compatible C library
featuring:

* a brand new [Redis protocol](http://redis.io/topics/protocol) parser written with the [Ragel](http://www.complang.org/ragel/) state machine compiler,
* a dedicated server layer written on-top of [libuv](https://github.com/joyent/libuv),
* an extensible backend layer via a Redis API C-based skeleton,
* [Tokyo Cabinet](http://fallabs.com/tokyocabinet/) as default backend.

You can use it with or without the server-stack, i.e in an embedded fashion by
taking benefit of the `rk_tcdb_t` C programmatic API.

The backend layer has been inspired from another similar project called [lycadb](https://github.com/nicolasff/lycadb) by [yowgi](http://twitter.com/yowgi).

## Use cases

It should be interesting for you if you want to run/embedd Redis e.g:

* on a mobile device with limited RAM, e.g.:
  * as an alternative embedded data store, to manipulate data the Redis way,
  * to implement a general purpose on-disk URL cache,
  * to implement a persistent store used to collect client-side logs, tasks, etc to be handled asynchronously,
* on a development machine on which a production dataset may not fit in memory,
* on a production machine:
  * as a backend to collect a very large amount of logs (e.g. see [Fluent event collector](http://fluentd.org/doc/overview.html)),
  * as a slave dedicated to disk-based replication.

## Install

First you need to install the [Ragel](http://www.complang.org/ragel/) compiler. If you are on OS X and use [homebrew](https://github.com/mxcl/homebrew) just type:

    brew install ragel
    
Then clone/get the dependencies ([libuv](https://github.com/joyent/libuv) and [Tokyo Cabinet 1.4.47](http://fallabs.com/tokyocabinet/)) by typing:

    make get-deps
    
At last compile `redisk` by typing:

    make

> **TIP** for a recap type `make help`.
    
## Usage

On a terminal, start the `redisk` server on the port of your choice (e.g. 1981) by typing:

    ./redisk 1981
    listening on port 1981
    
You can then use the `redis-cli` command line client to interact with `redisk`, e.g. within another terminal:

    $ redis-cli -p 1981 SET mykey Hello
    OK
    $ redis-cli -p 1981 GET mykey
    "Hello"

## C API Example Code

    #include <stdio.h>
    #include <stdlib.h>
    #include <stdbool.h>
    #include <stdint.h>
    
    #include "tcdb.h"
    
    int main(int argc, char **argv) {
        rk_tcdb_t *db;
        bool diag;
        char *res;
        int len;
        
        /* Initialize the database */
        db = rk_tcdb_new();
        if (!rk_tcdb_open(db, "rk.db")) {
            fprintf(stderr, "open error\n");
        }
        
        /* SET key value */
        diag = rk_tcdb_set(db, "mykey", 5, "Hello", 5);
        printf(" SET mykey \"Hello\" -> %s\n", diag ? "OK" : "(error)");
        
        /* GET key */
        res = rk_tcdb_get(db, "mykey", 5, &len);
        printf(" GET mykey -> %s\n", res ? res : "(nil)");
        
        /* Cleanup */
        if (!rk_tcdb_close(db)) {
            fprintf(stderr, "close error\n");
        }
        rk_tcdb_free(db);
        if (res) free(res);
        
        return 0;
    }

## Contributing

The project is still very young and has many room for improvements, e.g:

* write the missing Redis API commands (e.g. [LINDEX](http://redis.io/commands/lindex), [SINTER](http://redis.io/commands/sinter), etc),
* extend C API to support variadic commands (e.g. [HMGET](http://redis.io/commands/hmget)),
* introduce error codes to properly bubble up errors,
* write documentation,
* write tests,
* plug in another DBM backend, e.g:
  * [sqlite](http://www.sqlite.org/),
  * [Berkeley DB](http://www.oracle.com/technology/products/berkeley-db),
  * [leveldb](http://code.google.com/p/leveldb/),
  * etc
* improve the build system,
* packaging, i.e create a static library that wraps the backend layer to make it easy to embed,
* introduce parameters to tune things up (e.g Tokyo bucket number, cache size, etc),
* support [zsets](http://redis.io/commands#sorted_set) (low priority),
* etc

Known bugs:

* probably lots of memory leaks & issues to fix with Valgrind

If you enjoy it, feel free to fork and send your pull requests!

## Hack Day Paris

This project has been created at [Hack Day Paris](http://hackdayparis.org/2011.html) - a Paris based 40-hour marathon of hyper-intensive design and development.

## Authors

[Pierre Chapuis](http://twitter.com/pchapuis) & [Cédric Deltheil](http://about.me/deltheil).
