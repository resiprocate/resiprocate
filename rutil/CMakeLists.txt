set(INCLUDES
   compat.hxx
   ResipAssert.h
   FileSystem.hxx
   HashMap.hxx
   wince/WceCompat.hxx
   SysLogStream.hxx
   AsyncID.hxx
   AsyncBool.hxx
   ConfigParse.hxx
   CongestionManager.hxx
   GeneralCongestionManager.hxx
   HeapInstanceCounter.hxx
   KeyValueStore.hxx
   FdSetIOObserver.hxx
   Fifo.hxx
   CircularBuffer.hxx
   FiniteFifo.hxx
   ParseBuffer.hxx
   Log.hxx
   ThreadIf.hxx
   WinLeakCheck.hxx
   Random.hxx
   AsyncProcessHandler.hxx
   RADIUSDigestAuthenticator.hxx
   RWMutex.hxx
   Subsystem.hxx
   Logger.hxx
   MediaConstants.hxx
   MD5Stream.hxx
   DnsUtil.hxx
   Timer.hxx
   DigestStream.hxx
   TransportType.hxx
   resipfaststreams.hxx
   Coders.hxx
   Sha1.hxx
   SelectInterruptor.hxx
   Socket.hxx
   dns/ExternalDnsFactory.hxx
   dns/DnsStub.hxx
   dns/DnsHostRecord.hxx
   dns/QueryTypes.hxx
   dns/RROverlay.hxx
   dns/AresCompat.hxx
   dns/ExternalDns.hxx
   dns/DnsNaptrRecord.hxx
   dns/RRList.hxx
   dns/LocalDns.hxx
   dns/RRFactory.hxx
   dns/DnsSrvRecord.hxx
   dns/DnsCnameRecord.hxx
   dns/AresDns.hxx
   dns/RRCache.hxx
   dns/DnsHandler.hxx
   dns/RRVip.hxx
   dns/DnsAAAARecord.hxx
   dns/DnsResourceRecord.hxx
   Condition.hxx
   WinCompat.hxx
   vthread.hxx
   ServerProcess.hxx
   Data.hxx
   Lock.hxx
   TimeLimitFifo.hxx
   Mutex.hxx
   NetNs.hxx
   GenericTimerQueue.hxx
   IntrusiveListElement.hxx
   ssl/SHA1Stream.hxx
   ssl/OpenSSLDeleter.hxx
   ssl/OpenSSLInit.hxx
   CountStream.hxx
   Crc32.hxx
   vmd5.hxx
   XMLCursor.hxx
   PoolBase.hxx
   Plugin.hxx
   FdPoll.hxx
   Time.hxx
   stun/Udp.hxx
   stun/Stun.hxx
   SysLogBuf.hxx
   Inserter.hxx
   DataStream.hxx
   GenericIPAddress.hxx
   AbstractFifo.hxx
   AndroidLogger.hxx
   ParseException.hxx
   BaseException.hxx
   DataException.hxx
   Poll.hxx
   StlPoolAllocator.hxx
   ProducerFifoBuffer.hxx
   DinkyPool.hxx
   ConsumerFifoBuffer.hxx
   ProtonThreadBase.hxx
   PyExtensionBase.hxx
   PyThreadSupport.hxx
   hep/HepAgent.hxx
   hep/ResipHep.hxx
   rtcp/re_rtp.h
)

add_library(rutil
   AbstractFifo.cxx
   AndroidLogger.cxx
   BaseException.cxx
   Coders.cxx
   ConfigParse.cxx
   CountStream.cxx
   Crc32.cxx
   ServerProcess.cxx
   Data.cxx
   DataStream.cxx
   DnsUtil.cxx
   FileSystem.cxx
   GeneralCongestionManager.cxx
   GenericIPAddress.cxx
   HeapInstanceCounter.cxx
   KeyValueStore.cxx
   Lock.cxx
   Log.cxx
   MD5Stream.cxx
   NetNs.cxx
   ParseBuffer.cxx
   ParseException.cxx
   Poll.cxx
   PoolBase.cxx
   FdPoll.cxx
   RADIUSDigestAuthenticator.cxx
   RWMutex.cxx
   Random.cxx
   resipfaststreams.cxx
   SelectInterruptor.cxx
   Sha1.cxx
   Socket.cxx
   Subsystem.cxx
   SysLogBuf.cxx
   SysLogStream.cxx
   ThreadIf.cxx
   Time.cxx
   Timer.cxx
   TransportType.cxx
   vmd5.cxx
   XMLCursor.cxx

   dns/AresDns.cxx
   dns/DnsCnameRecord.cxx
   dns/DnsAAAARecord.cxx
   dns/DnsHostRecord.cxx
   dns/DnsNaptrRecord.cxx
   dns/DnsResourceRecord.cxx
   dns/DnsThread.hxx
   dns/DnsSrvRecord.cxx
   dns/DnsStub.cxx
   dns/DnsThread.cxx
   dns/ExternalDnsFactory.cxx
   dns/RRCache.cxx
   dns/RRList.cxx
   dns/RRVip.cxx
   dns/QueryTypes.cxx
   dns/RROverlay.cxx

   stun/Stun.cxx
   stun/Udp.cxx

   hep/HepAgent.cxx
   hep/ResipHep.cxx

   ${INCLUDES}
)

target_add_conditional_sources(rutil OPENSSL_FOUND
   ssl/OpenSSLDeleter.cxx
   ssl/OpenSSLInit.cxx
   ssl/SHA1Stream.cxx
)

target_add_conditional_sources(rutil BUILD_QPID_PROTON
   ProtonThreadBase.cxx
)

target_add_conditional_sources(rutil BUILD_DSO_PLUGINS
   Plugin.cxx
)

function(copy_pycxx)
   get_filename_component(BASE_FILENAME ${ARGV0} NAME)
   add_custom_command(
      OUTPUT ${BASE_FILENAME}
      COMMAND ${CMAKE_COMMAND} -E copy ${PYCXX_SRCDIR}/${ARGV0} .
   )
endfunction()
if(BUILD_PYTHON)
   copy_pycxx(Python3/cxx_exceptions.cxx)
   copy_pycxx(Python3/cxxextensions.c)
   copy_pycxx(Python3/cxx_extensions.cxx)
   copy_pycxx(Python3/cxxsupport.cxx)
   copy_pycxx(IndirectPythonInterface.cxx)
endif()

target_add_conditional_sources(rutil BUILD_PYTHON
   PyExtensionBase.cxx
   cxxextensions.c
   cxx_extensions.cxx
   cxxsupport.cxx
   IndirectPythonInterface.cxx
   cxx_exceptions.cxx
)

set_source_files_properties(
   cxxextensions.c
   cxx_extensions.cxx
   cxxsupport.cxx
   IndirectPythonInterface.cxx
   cxx_exceptions.cxx
   PROPERTIES GENERATED TRUE
)

target_add_conditional_sources(rutil WIN32
   #wince/WceCompat.cxx
   WinCompat.cxx
)

# %HOME...%\source\repos\resiprocate\builds\packages\zeroc.openssl.v142\build\native\bin\x64\Debug

if(WIN32)
   target_link_libraries(rutil PUBLIC winmm)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   target_link_libraries(rutil PUBLIC rt)
endif()

target_include_directories(rutil PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)
target_include_directories(rutil PRIVATE ${ARES_INCLUDE_DIRS})
target_include_directories(rutil PUBLIC ${QPIDPROTON_INCLUDE_DIRS})
target_include_directories(rutil PUBLIC ${Python3_INCLUDE_DIRS} ${PYCXX_INCLUDE_DIRS})
target_include_directories(rutil PUBLIC ${OPENSSL_INCLUDE_DIR})
if(USE_NUGET)
  target_link_libraries(rutil PUBLIC OPENSSL)
  target_link_libraries(rutil PUBLIC OPENSSL_CRYPTO)
else()
  target_link_libraries(rutil PUBLIC ${OPENSSL_LIBRARIES})
endif()
target_link_libraries(rutil PRIVATE ${ARES_LIBRARIES})
target_link_libraries(rutil PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(rutil PUBLIC ${QPIDPROTON_LIBRARIES})
target_link_libraries(rutil PUBLIC ${Python3_LIBRARIES} ${PYCXX_LIBRARIES})
target_link_libraries(rutil PUBLIC ${LIBRADIUS_LIBRARIES})
target_link_libraries(rutil PUBLIC ${FMT_LIBRARIES})
if(ENABLE_ANDROID)
   target_link_libraries(rutil PUBLIC log)
endif()
set_target_properties(rutil PROPERTIES FOLDER rutil)
version_libname(rutil)

install(TARGETS rutil DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/rutil ${INCLUDES})

if(NOT WITH_C_ARES)
   add_subdirectory(dns/ares)
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
