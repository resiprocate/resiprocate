#!/bin/sh
awk '{printf("%s\r\n",$0);}' $*
