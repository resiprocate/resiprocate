#!/bin/bash

# This is in two parts.
# 1. Run lots of times to make some data.
# 2. Plot the data.
# Can override the test driver with --driver="./testFnoo --arg1 --arg2=x "
# Will always append --num-runs
#
# In general, you want to run this with --generate NAME.

# Once you have some data in perfdata/*, you can run this with --plot.

# Make data -- each run makes a file called 'NAME'

driver=./testStack\ --bind=127.0.0.1


while [ $# -gt 1 ]; do
    case $1 in
        --dri*)
            driver=${1##--*=}
            ;;
        --gen*)
            if [ $# -ne 3 ]; then
                echo usage: $0 --generate name num
                exit -1
            fi
            # check for test driver
            if [ ! -x ./testStack ]; then
                echo $driver not executable or missing.
                exit -2
            fi
            [ -d data ] || mkdir data
            name=$2
            nruns=$3
            output="data/$name"
            if [ -f "$output" ]; then
                echo "$output: exists, please choose alternate name or remove file"
                exit -3
            fi
            while [ $nruns -gt 0 ]; do
                # cross plat random number
                nr=$( dd if=/dev/random count=1 bs=4 2> /dev/null | od -d |\
                    awk '/^000* /,// {print $2$3;}')
                nr=$(( $nr % 100000 + 1))
                echo runs left $nruns -- this run for $nr REGISTERs
                $driver --num-runs=$nr | awk '/registrations per?formed/ { print $1 " " $5 " "$10;}'  >> "$output"
                nruns=$(( $nruns - 1 ))
            done
            ;;
        --pl*)
            echo plot mode
            ;;
        *)
            ;;
    esac
    shift
done

