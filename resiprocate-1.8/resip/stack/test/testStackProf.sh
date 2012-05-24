#!/bin/bash

ARCH=bin.prof.Linux.x86_64
PROG=$ARCH/testStack

rm -f gmon.out
$PROG -r 50000 -p tcp -t poll -n 10000 --bind=127.0.0.1 --listen=1
gprof -k __do_global_ctors_aux/threadIfThreadWrapper $PROG > a
