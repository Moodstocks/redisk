#!/bin/sh

function rcli {
  ../redis/src/redis-cli -p 8000 $*
}

# Errors

echo "=> invalid"
rcli XET a

echo "=> arity"
rcli SET a

# Strings

echo "=> set"
rcli SET kfoo vfoo

echo "=> setnx"
rcli SETNX kbar vbar
rcli SETNX kfoo vuseless

echo "=> get"
rcli GET kfoo
rcli GET kdoesnotexist

echo "=> exists"
rcli EXISTS kfoo
rcli EXISTS kdoesnotexist

echo "=> getset"
rcli GETSET kfoo vfoo2
rcli GETSET kfoo vfoo

echo "=> type"
rcli TYPE kfoo
rcli TYPE kdoesnotexist

echo "=> del"
rcli DEL kfoo
rcli GET kfoo

# Counters

echo "=> counters"
rcli INCR cfoo
rcli INCR cfoo
rcli INCR cfoo
rcli DECR cfoo
rcli INCRBY cfoo 3
rcli DECRBY cfoo 2

# Hashes

echo "=> hset"
rcli HSET hfoo ffoo vfoofoo
rcli HSET hfoo fbar vfoobar

echo "=> hget"
rcli HGET hfoo fbar
rcli HGET hfoo fdoesnotexist

echo "=> hexists"
rcli HEXISTS hfoo fbar
rcli HEXISTS hfoo fdoesnotexist

echo "=> hlen"
rcli HLEN hfoo

echo "=> hdel"
rcli HDEL hfoo ffoo
rcli HEXISTS hfoo ffoo
rcli HLEN hfoo

echo "=> hsetnx"
rcli HSETNX hfoo fbar vuseless
rcli HSETNX hfoo ffoo vfoofoo
rcli HGET hfoo ffoo

# Sets

echo "=> sadd"
rcli SADD sfoo v1
rcli SADD sfoo v2
rcli SADD sfoo v3

echo "=> sismember"
rcli SISMEMBER sfoo v2
rcli SISMEMBER sfoo vnone

echo "=> srem"
rcli SREM sfoo v2
rcli SISMEMBER sfoo v2

echo "=> scard"
rcli SCARD sfoo

# Lists

echo "=> lpush"
rcli LPUSH lfoo v1
rcli LPUSH lfoo v2
rcli LPUSH lfoo v3
rcli LPUSH lfoo v4
rcli LPUSH lfoo v5

echo "=> lpop"
rcli LPOP lfoo
rcli LPOP lfoo
rcli LPOP ldoesnotexist

echo "=> llen"
rcli LLEN lfoo
rcli LLEN ldoesnotexist

echo "=> rpop"
rcli RPOP lfoo
rcli RPOP lfoo
rcli RPOP ldoesnotexist
rcli RPOP lfoo
rcli RPOP lfoo

echo "=> rpush"
rcli RPUSH lfoo v2
rcli LPUSH lfoo v3
rcli RPUSH lfoo v1
rcli LPUSH lfoo v4
rcli RPOP lfoo
rcli RPOP lfoo
rcli RPOP lfoo
rcli RPOP lfoo

echo "=> lrange"
rcli RPUSH mylist one
rcli RPUSH mylist two
rcli RPUSH mylist three
rcli LRANGE mylist 0 0
rcli LRANGE mylist -3 2
rcli LRANGE mylist -100 100
rcli LRANGE mylist 5 10
