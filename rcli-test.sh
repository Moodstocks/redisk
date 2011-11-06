#!/bin/sh

function rcli {
  ../redis/src/redis-cli -p 8000 $*
}

echo "=> invalid"
rcli XET a

echo "=> arity"
rcli SET a

echo "=> set"
rcli SET kfoo vbar

echo "=> get"
rcli GET kfoo
rcli GET kdoesnotexist

echo "=> exists"
rcli EXISTS kfoo
rcli EXISTS kdoesnotexist

echo "=> type"
rcli TYPE kfoo
rcli KTYPE kdoesnotexist

echo "=> del"
rcli DEL kfoo
rcli GET kfoo

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

