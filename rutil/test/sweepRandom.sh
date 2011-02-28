#!/bin/bash
set -e

# Basic idea to use on POSIX platforms:
# 1. Build default tree (rutil & rutil/tests) with optmization
# 2. cp tests/testRandomThread to tests/testRandomThread.random
# 3. Change Random.hxx to define THREAD_MUTEX, rebuild and copy to testRandomThread.mutex
# 3. Change Random.hxx to define THREAD_LOCAL, rebuild and copy to testRandomThread.local

numCycles=10

ProgBase=./testRandomThread
KnownFlavors="none random mutex local"
RunFlavors="random mutex local"

for flavor in $KnownFlavors; do
    if [ "$1" = $flavor ] ; then
	RunFlavors=$flavor
	shift
	continue
    fi
done

if [ "$1" != "" ] ; then
    echo "Uknown option: $1"
    exit 1
fi

for flavor in $RunFlavors; do
    Prog=$ProgBase
    if [ $flavor != "none" ] ; then
        Prog=$ProgBase.$flavor
    fi
    if [ ! -x $Prog ] ; then
	echo "Program $Prog is missing; skipping..."
	continue
    fi

    echo "Sweeping $Prog ..."
    for numThreads in -1 0 1 3 9 27; do
        $Prog $numCycles $numThreads
    done
done
