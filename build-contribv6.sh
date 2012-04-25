#!/bin/bash

cd contrib/ares && ./configure "CFLAGS=-DUSE_IPV6=1 -fPIC -g -O2 -Wall" && make


