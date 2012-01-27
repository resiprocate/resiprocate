INCLUDE_DIRECTORIES(${RESIPROCATE_BINARY_DIR} ${RESIPROCATE_BINARY_DIR}/contrib/ares ${RESIPROCATE_BINARY_DIR}/repro-deps/log4cplus-1.0.2/include )
FOREACH(program  	testCoders.cxx testCountStream.cxx testData.cxx testDataPerformance.cxx testDataStream.cxx testDnsUtil.cxx testFifo.cxx testFileSystem.cxx testInserter.cxx testIntrusiveList.cxx testLogger.cxx testMD5Stream.cxx testParseBuffer.cxx testRandomHex.cxx testSelectInterruptor.cxx testSHA1Stream.cxx testThreadIf.cxx )
  GET_FILENAME_COMPONENT(progname ${program} NAME_WE)
  ADD_EXECUTABLE(${progname} ${program})
  TARGET_LINK_LIBRARIES(${progname} ${RESIPROCATE_LIBS} )
ENDFOREACH( program)

# Copyright 2007 Estacado Systems
