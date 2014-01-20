INCLUDE_DIRECTORIES(../..)

FUNCTION(TEST)
   TEST_BASE(${ARGV})
   SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES FOLDER reTurn/client/Tests)
   TARGET_LINK_LIBRARIES(${ARGV0} reTurnClient rutil ${OPENSSL_LIBRARIES})
ENDFUNCTION()


TEST(TestAsyncClient TestAsyncClient.cxx)
TEST(TestRtpLoad TestRtpLoad.cxx)
TEST(TestClient TestClient.cxx)
