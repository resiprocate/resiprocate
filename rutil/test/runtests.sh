#!/bin/bash

function die() {
    echo "$1" >&2;
    exit 1;
}

failed=0;


for i in \
	testCoders \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDnsUtil \
	testFifo \
	testFileSystem \
	testInserter \
	testIntrusiveList \
	testKeyValueStore \
	testLogger \
	testMD5Stream \
	testRandomHex \
	testSHA1Stream \
	testParseBuffer \
	testThreadIf \
	testXMLCursor;
do
    if test ! -x $i; then
        echo "$i: test does not exist" >&2;
    else
        ./$i >$i.log 2>&1;
        if test $? -ne 0; then
            die "$i: test failed";
            failed=`expr ${failed} + 1`;
        fi;
        echo "$i: test passed";
    fi;
done;

if test ${failed} -ne 0; then
    echo "${failed} tests failed" >&2;
    exit 1;
fi;

#	testMem \
