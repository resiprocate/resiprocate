#!/bin/bash

function die() {
    echo "$1" >&2;
    exit 1;
}

function pad () {
    local l=$( echo -n $2 | wc -c )
    local o=""
    while [ $l -lt $1 ]; do
	o="${o} "
	l=$(( $l + 1 ))
    done
    echo -n "${o}${2} : " >& 2
}

failed=0


drivers="
	testAppTimer 
	testApplicationSip 
	testConnectionBase 
	testCorruption 
	testDigestAuthentication 
	testEmbedded 
	testEmptyHeader 
	testExternalLogger 
	testIM 
	testMessageWaiting 
	testMultipartMixedContents 
	testMultipartRelated 
	testParserCategories 
	testPidf 
	testPksc7 
	testPlainContents 
	testRlmi 
	testSdp 
	testSelectInterruptor 
	testSipFrag 
	testSipMessage 
	testSipMessageMemory 
	testStackStd.sh
	testTcp 
	testTime 
	testTimer	 
	testTuple 
	testUri"

echo top

x=0
length=0

for prg in ${drivers}; do
    x=$(( $(echo $prg  | wc -c ) + 0 ))
    [ $x"x" == "x" ] && continue
    if [ $x -gt $length ]; then
	length=$x
    fi
done

flist=""
mlist=""
failed=0
missed=0

for i in ${drivers}; do
  t=./$i
  pad $length $i
    if test ! -x $t; then
        echo "not found. (fail)" >&2
	mlist="${mlist} ${i}"
	missed=$(( $missed + 1 )) 
    else
        if $t > $i.log 2>&1; then
            echo "passed" >&2
	else
	    echo "failed" >&2
            failed=$(( $failed + 1 ))
	    flist="${flist} ${i}"
        fi
    fi;
done;


if [ ${missed} -ne 0 ]; then
    echo "Missed: "${mlist}
    echo "${missed} tests missing" >&2
fi

if [ ${failed} -ne 0 ]; then
    [ ${missed} -ne 0 ] && echo ""
    echo "Failed: "${flist}
    echo "${failed} tests failed" >&2
fi


exit $(( ${missed} + ${failed} ))

