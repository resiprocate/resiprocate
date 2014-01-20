INCLUDE_DIRECTORIES(../..)

FUNCTION(TEST)
   TEST_BASE(${ARGV})
   SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES FOLDER reflow/Tests)
   TARGET_LINK_LIBRARIES(${ARGV0} reflow rutil ${OPENSSL_LIBRARIES} ${SRTP_LIBRARIES})
ENDFUNCTION()

SET(SHARED_SRC
   CreateCert.cxx
   TestDtlsUdp.cxx
   TestTimerContext.cxx
)

TEST(testDtlsWrapper testDtlsWrapper.cxx ${SHARED_SRC})
TEST(testDtlsClient testDtlsClient.cxx ${SHARED_SRC})
TEST(testDtlsServer testDtlsServer.cxx ${SHARED_SRC})
