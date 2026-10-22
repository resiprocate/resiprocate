set(SHARED_SRC
   CreateCert.cxx
   TestDtlsUdp.cxx
   TestTimerContext.cxx
)

if(WITH_SSL)
   test(testDtlsWrapper testDtlsWrapper.cxx)
   # testDtlsServer <port> and testDtlsClient <host> <port> exchange DTLS-SRTP over
   # UDP; lines typed into the client are echoed back upper-cased by the server
   manual_test(testDtlsClient testDtlsClient.cxx ${SHARED_SRC})
   manual_test(testDtlsServer testDtlsServer.cxx ${SHARED_SRC})
endif()
