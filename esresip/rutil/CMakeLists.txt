INCLUDE_DIRECTORIES(${RESIPROCATE_SOURCE_DIR} ${RESIPROCATE_SOURCE_DIR}/contrib/ares ${RESIPROCATE_SOURCE_DIR}/repro-deps/log4cplus-1.0.2/include )
SET(RUTIL_SOURCE AbstractFifo.cxx  BaseException.cxx  Coders.cxx  Condition.cxx  CountStream.cxx  Data.cxx  DataStream.cxx  DnsUtil.cxx  EsLogger.cxx  FileSystem.cxx  HeapInstanceCounter.cxx  Lock.cxx  Log.cxx  Logger.cxx  MD5Stream.cxx  Mutex.cxx  OpenSSLInit.cxx  ParseBuffer.cxx  Poll.cxx  RWMutex.cxx  Random.cxx  RecursiveMutex.cxx  ResipLogAdapter.cxx  SelectInterruptor.cxx  SHA1Stream.cxx  SimpleXMLElement.cxx  Socket.cxx  Subsystem.cxx  SysLogBuf.cxx  SysLogStream.cxx  ThreadIf.cxx  Timer.cxx  TransportType.cxx  vmd5.cxx  XMLLayout.cxx dns/DnsAAAARecord.cxx  dns/DnsCnameRecord.cxx  dns/DnsHostRecord.cxx  dns/DnsNaptrRecord.cxx  dns/DnsSrvRecord.cxx  dns/DnsStub.cxx  dns/ExternalDnsFactory.cxx  dns/RRCache.cxx  dns/RRList.cxx  dns/RRVip.cxx  dns/QueryTypes.cxx  dns/RROverlay.cxx   stun/Stun.cxx  stun/Udp.cxx )
SET(ARES_SOURCE dns/AresDns.cxx)
IF(USE_ARES)
   ADD_LIBRARY(rutil ${ARES_SOURCE} ${RUTIL_SOURCE})
ELSE(USE_ARES)
  ADD_LIBRARY(rutil ${RUTIL_SOURCE})
ENDIF(USE_ARES)

INSTALL(TARGETS rutil
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

IF(BUILD_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF(BUILD_TESTS)

# Copyright 2007 Estacado Systems
