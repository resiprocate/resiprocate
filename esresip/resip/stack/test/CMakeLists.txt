INCLUDE_DIRECTORIES(${RESIPROCATE_BINARY_DIR} ${RESIPROCATE_BINARY_DIR}/contrib/ares ${RESIPROCATE_BINARY_DIR}/repro-deps/log4cplus-1.0.2/include )
SET(TEST_SOURCES  
SipTortureTests.cxx 
limpc.cxx 
test503Generator.cxx 
testAppTimer.cxx 
testApplicationSip.cxx 
testClient.cxx 
testConnectionBase.cxx 
testCorruption.cxx 
testDigestAuthentication.cxx 
testDtlsTransport.cxx 
testDns.cxx 
testEmbedded.cxx 
testEmptyHeader.cxx 
testExternalLogger.cxx 
testFlowId.cxx 
testIM.cxx 
testLockStep.cxx 
testMessageWaiting.cxx 
testMultipartMixedContents.cxx 
testMultipartRelated.cxx 
testParserCategories.cxx 
testPidf.cxx 
testPidf.cxx 
testPksc7.cxx 
testPlainContents.cxx 
testRSP-2.cxx 
testResponses.cxx 
testRlmi.cxx 
testSdp.cxx 
testSelect.cxx 
testServer.cxx 
testSipFrag.cxx 
testSipMessage.cxx 
testSipMessageMemory.cxx 
testSipStack1.cxx 
testStack.cxx 
testTcp.cxx 
testTime.cxx 
testTimer.cxx 
testTransactionFSM.cxx 
testTuple.cxx 
testTypedef.cxx 
testUdp.cxx 
testUri.cxx 
testXMLCursor.cxx 
)

IF(USE_SSL)
  SET(TEST_SOURCES ${TEST_SOURCES} 
                   makeSelfCert.cxx 
                   testSecurity.cxx 
                   testTls.cxx )
ENDIF(USE_SSL)

FOREACH(program ${TEST_SOURCES})
  GET_FILENAME_COMPONENT(progname ${program} NAME_WE)
  ADD_EXECUTABLE(${progname} ${program} TestSupport.cxx)
  TARGET_LINK_LIBRARIES(${progname} ${RESIPROCATE_LIBS} )
ENDFOREACH( program)

# Copyright 2007 Estacado Systems
