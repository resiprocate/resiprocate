# Used for installation
SET(INCLUDES
   AbstractFifo.hxx
   AndroidLogger.hxx
   AsyncID.hxx
   AsyncProcessHandler.hxx
   BaseException.hxx
   CircularBuffer.hxx
   Coders.hxx
   Condition.hxx
   ConfigParse.hxx
   CongestionManager.hxx
   ConsumerFifoBuffer.hxx
   CountStream.hxx
   Data.hxx
   DataException.hxx
   DinkyPool.hxx
   DnsUtil.hxx
   FdPoll.hxx
   FdSetIOObserver.hxx
   Fifo.hxx
   FileSystem.hxx
   FiniteFifo.hxx
   GeneralCongestionManager.hxx
   GenericIPAddress.hxx
   GenericTimerQueue.hxx
   HashMap.hxx
   HeapInstanceCounter.hxx
   Inserter.hxx
   IntrusiveListElement.hxx
   KeyValueStore.hxx
   Lock.hxx
   Lockable.hxx
   Log.hxx
   Logger.hxx
   MD5Stream.hxx
   Mutex.hxx
   ParseBuffer.hxx
   ParseException.hxx
   Poll.hxx
   PoolBase.hxx
   ProducerFifoBuffer.hxx
   RADIUSDigestAuthenticator.hxx
   RWMutex.hxx
   Random.hxx
   RecursiveMutex.hxx
   SelectInterruptor.hxx
   ServerProcess.hxx
   SharedCount.hxx
   SharedPtr.hxx
   Socket.hxx
   StlPoolAllocator.hxx
   Subsystem.hxx
   SysLogBuf.hxx
   SysLogStream.hxx
   ThreadIf.hxx
   Time.hxx
   TimeLimitFifo.hxx
   Timer.hxx
   TransportType.hxx
   XMLCursor.hxx
   compat.hxx
   resipfaststreams.hxx
   vmd5.hxx
   vthread.hxx
)

SET(INCLUDES_WIN
   WinCompat.hxx
   WinLeakCheck.hxx
)

SET(INCLUDES_SSL
   DigestStream.hxx
)

# Special case because some SSL files are under "ssl"
SET(INCLUDES_SSL_FOLDER
   ssl/OpenSSLInit.hxx
   ssl/SHA1Stream.hxx
)

# Special case because some stun files are under "stun"
# TODO: Protect these using conditional compile? How does the autotools build
# manage it?
SET(INCLUDES_STUN_FOLDER
   stun/Stun.hxx
   stun/Udp.hxx
)

# TODO: Have a special check for Android and don't include AndroidLogger
# everywhere...

# Used for compilation
SET(SOURCES
   AbstractFifo.cxx
   AndroidLogger.cxx
   BaseException.cxx
   Coders.cxx
   Condition.cxx
   ConfigParse.cxx
   CountStream.cxx
   Data.cxx
   DataStream.cxx
   dns/AresDns.cxx
   dns/DnsAAAARecord.cxx
   dns/DnsCnameRecord.cxx
   dns/DnsHostRecord.cxx
   dns/DnsNaptrRecord.cxx
   dns/DnsResourceRecord.cxx
   dns/DnsSrvRecord.cxx
   dns/DnsStub.cxx
   dns/DnsThread.cxx
   dns/ExternalDnsFactory.cxx
   dns/LocalDns.cxx
   dns/QueryTypes.cxx
   dns/RRCache.cxx
   dns/RRList.cxx
   dns/RROverlay.cxx
   dns/RRVip.cxx
   DnsUtil.cxx
   FdPoll.cxx
   FileSystem.cxx
   GeneralCongestionManager.cxx
   HeapInstanceCounter.cxx
   KeyValueStore.cxx
   Lock.cxx
   Log.cxx
   MD5Stream.cxx
   Mutex.cxx
   ParseBuffer.cxx
   ParseException.cxx
   Poll.cxx
   PoolBase.cxx
   RADIUSDigestAuthenticator.cxx
   Random.cxx
   RecursiveMutex.cxx
   resipfaststreams.cxx
   RWMutex.cxx
   SelectInterruptor.cxx
   ServerProcess.cxx
   Socket.cxx
   stun/Stun.cxx
   stun/Udp.cxx
   Subsystem.cxx
   SysLogBuf.cxx
   SysLogStream.cxx
   ThreadIf.cxx
   Time.cxx
   Timer.cxx
   TransportType.cxx
   vmd5.cxx
   XMLCursor.cxx
)

# TODO: Add WceCompat.cxx on WinCE
SET(SOURCES_WIN
   #wince/WceCompat.cxx
   WinCompat.cxx
)

SET(SOURCES_SSL
# TODO DigestStream.cxx doesn't seem to work on Windows... fix it or remove
# file if not used at all?
#   DigestStream.cxx
   ssl/OpenSSLInit.cxx
   ssl/SHA1Stream.cxx
)

# If compiling with OpenSSL support, add extra SSL related source and header
# files
IF(${OPENSSL_FOUND})
   LIST(APPEND SOURCES ${SOURCES_SSL}) 
   LIST(APPEND INCLUDES ${INCLUDES_SSL})
   LIST(APPEND INCLUDES ${INCLUDES_SSL_FOLDER})

   # Since some ssl files are under the ssl folder they require their own
   # INSTALL command
   INSTALL(FILES ${INCLUDES_SSL_FOLDER} DESTINATION include/rutil/ssl)
ENDIF()

LIST(APPEND INCLUDES ${INCLUDES_STUN_FOLDER})

# TODO: Remove Windows .hxx header files from INSTALL if not Windows

# TODO: Use SOURCE_GROUP to properly represent the subfolders in MS Visual
# Studio. For example stun/Stun.?xx is shown under VS in the root of the source
# tree, omitting the stun subfolder.

INCLUDE_DIRECTORIES(..)

# Whether or not to build resip-ares
IF(NOT ${WITH_C_ARES})
   ADD_SUBDIRECTORY(dns/ares)
ENDIF()

# OPENSSL_LIBRARIES will be empty if SSL is not used
SET(TARGET_LIBS ${OPENSSL_LIBRARIES} ${ARES_LIBRARIES})

IF(CMAKE_HOST_WIN32)
   # Static library on Windows
   SET(TARGET_TYPE "")

   # Also add Windows-related source and header files
   LIST(APPEND SOURCES ${SOURCES_WIN})
   LIST(APPEND INCLUDES ${INCLUDES_WIN})

   LIST(APPEND TARGET_LIBS winmm)
ELSE()
   # Shared library on Unix
   SET(TARGET_TYPE SHARED)
   
   IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      LIST(APPEND TARGET_LIBS rt)
   ENDIF()
ENDIF()

ADD_LIBRARY(rutil ${TARGET_TYPE} ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(rutil ${TARGET_LIBS})
SET_TARGET_PROPERTIES(rutil PROPERTIES FOLDER rutil)

INSTALL(TARGETS rutil DESTINATION lib)
INSTALL(FILES ${INCLUDES} DESTINATION include/rutil)

# Since some stun files are under the stun folder they require their own
# INSTALL command
INSTALL(FILES ${INCLUDES_STUN_FOLDER} DESTINATION include/rutil/stun)

ADD_SUBDIRECTORY(test)
