#!/bin/sh
F=$1
YAY="Success!"

echo "$0: grepping for $YAY in file $F";
R=`grep $YAY $F`
if [ "$R" = "" ]; then
  exit 1;
else
  exit 0;
fi

