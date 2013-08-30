#!/system/bin/sh  

# To use this script:

# cp /sdcard/run.sh /data/run.sh
# cd /data
# chmod 0777 run.sh
# ./run.sh

for i in /data/resip-test /data/resip-test/bin /data/resip-test/lib
do
  mkdir -p $i
  chmod 0777 $i
done

cp /sdcard/resip-test/lib/* /data/resip-test/lib
cp /sdcard/resip-test/bin/* /data/resip-test/bin

chmod 0777 /data/resip-test/bin/*
chmod 0777 /data/resip-test/lib/*

RUTIL_TEST="testCoders \
        testCountStream \
        testData \
        testDataPerformance \
        testDataStream \
        testDnsUtil \
        testFifo \
        testFileSystem \
        testInserter \
        testIntrusiveList \
        testLogger \
        testMD5Stream \
        testParseBuffer \
        testRandomHex \
        testRandomThread \
        testThreadIf \
        testXMLCursor"

RESIP_TEST="testAppTimer \
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
        testParserCategories \
        testPidf \
        testPksc7 \
        testPlainContents \
        testRlmi \
        testSdp \
        testSelectInterruptor \
        testSipFrag \
        testSipMessage \
        testSipMessageMemory \
        testStack \
        testTcp \
        testTime \
        testTimer \
        testTuple \
        testUri"

DUM_TEST="basicRegister \
        BasicCall \
        basicMessage \
        basicClient \
        testRequestValidationHandler"

RESULT_FILE=/sdcard/resip-test/tests.log

rm -rf ${RESULT_FILE}

for test_case in $RUTIL_TEST $RESIP_TEST $DUM_TEST ;
do
  LD_LIBRARY_PATH=/data/resip-test/lib:/system/lib /data/resip-test/bin/$test_case
  echo "$? : $test_case" >> $RESULT_FILE
done

echo All done


