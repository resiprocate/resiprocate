function die() {
    echo "$1" >&2;
    exit 1;
}

failed=0;

for i in \
	testAppTimer \
	testApplicationSip \
	testConnectionBase \
	testCorruption \
	testDigestAuthentication \
	testEmbedded \
	testEmptyHeader \
	testExternalLogger \
	testIM \
	testMessageWaiting \
	testMultipartMixedContents \
	testMultipartRelated \
	testParseBuffer \
	testParserCategories \
	testPidf \
	testPksc7 \
	testPlainContents \
	testRlmi \
	testSdp \
	testSipFrag \
	testSipMessage \
	testSipMessageMemory \
	testStack \
	testTcp \
	testTime \
	testTimer	 \
	testTuple \
	testUri;
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

#	sipTortureTests
#	test503Generator
#	testDigestAuthentication2
#	testFlowId
#	testLockStep
#	testResponses
#	testUdp
#       testDns
