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

echo "=> get existing"
rcli GET kfoo

echo "=> get empty"
rcli GET kdoesnotexist

