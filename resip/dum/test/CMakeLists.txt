INCLUDE_DIRECTORIES(../..)

FUNCTION(TEST)
   TEST_BASE(${ARGV})
   IF(CMAKE_HOST_WIN32)
      TARGET_LINK_LIBRARIES(${ARGV0} rutil resiprocate dum)
   ELSE()
      TARGET_LINK_LIBRARIES(${ARGV0} rutil resip dum ${POPT_LIBRARIES})
   ENDIF()
   SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES FOLDER resip/Tests)   
ENDFUNCTION()

# TODO: A lot of files still depend on config.h, have CMake generate it if
# still needed
# TODO: Only enable tests that are already enabled by default... Some are not even compiling

SET(SHARED_SRC
   CommandLineParser.cxx
   RegEventClient.cxx
   UserAgent.cxx
   basicClientCall.cxx
   basicClientCmdLineParser.cxx
   basicClientUserAgent.cxx
)

#basicPublication.cxx
#testSMIMEInvite.cxx
#treg.cxx
#rlsListGenerator.cxx
#unreg.cxx
#rlsServer.cxx
#testRedirectManager.cxx
#testDSI.cxx
#testPayloadEncrypter.cxx
#certServer.cxx
#limpc.cxx
#curlHttp/testCurlHttpProvider.cxx
#curlHttp/CurlHttpProvider.cxx
#testSecurity.cxx
#testSMIMEMessage.cxx
#test3pcc.cxx
#testDumTimer.cxx
#testIdentity.cxx

TEST(BasicCall BasicCall.cxx ${SHARED_SRC})
TEST(basicRegister basicRegister.cxx ${SHARED_SRC})
TEST(basicMessage basicMessage.cxx ${SHARED_SRC})
TEST(basicClient basicClient.cxx ${SHARED_SRC})
TEST(testRequestValidationHandler testRequestValidationHandler.cxx ${SHARED_SRC})

