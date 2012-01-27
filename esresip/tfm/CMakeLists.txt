INCLUDE_DIRECTORIES(${RESIPROCATE_SOURCE_DIR} ${RESIPROCATE_SOURCE_DIR}/contrib/ares ${RESIPROCATE_SOURCE_DIR}/tfm/contrib/cppunit/include/ ${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/include/  ${RESIPROCATE_SOURCE_DIR}/)
SET(LIBTFM_SOURCES
        ActionBase.cxx 
        AsciiGraphic.cxx 
        Box.cxx 
        CommandLineParser.cxx 
        CheckContacts.cxx
        CheckFetchedContacts.cxx 
        CountDown.cxx 
        DialogSet.cxx 
        DnsUtils.cxx 
        Event.cxx 
        ExpectAction.cxx 
        ExpectActionEvent.cxx 
        Fixture.cxx 
        Renderer.cxx 
        Resolver.cxx 
        RouteGuard.cxx 
        Sequence.cxx 
        SequenceSet.cxx 
        SequenceSetThread.cxx 
        SipEvent.cxx 
        SipRawMessage.cxx 
        Source.cxx 
        TelnetClient.cxx 
        TestEndPoint.cxx 
        TestException.cxx 
        TestProxy.cxx 
        TestSipEndPoint.cxx 
        TestUser.cxx 
        TransportDriver.cxx 
        
        predicates/ExpectUtils.cxx 
        predicates/NotifyContents.cxx 
        predicates/SubscribeExpired.cxx 
        predicates/SubscribeExpires.cxx 
        
        repro/TestRepro.cxx 
        repro/TestReproUser.cxx 
)

IF(TEST_BASIS)
  SET(LIBTFM_SOURCES ${LIBTFM_SOURCES} 
                     sipbasis/SipBasisFixture.cxx 
                     sipbasis/SipBasisDriver.cxx 
                     sipbasis/MsgHandler.cxx 
     )
  ADD_EXECUTABLE(sipBasisTests sipBasisTests.cxx)
ENDIF(TEST_BASIS)

ADD_LIBRARY(tfm ${LIBTFM_SOURCES})

LINK_DIRECTORIES( ${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/src )
IF(NOT EXISTS ${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/Makefile)
  EXEC_PROGRAM( "perl"               "/${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/" 
ARGS "/${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/configure.pl" )
ENDIF(NOT EXISTS ${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/Makefile)
  
ADD_CUSTOM_TARGET("${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/src/libNetxx.a" ALL
                    COMMAND "make"
                    WORKING_DIRECTORY "${RESIPROCATE_SOURCE_DIR}/tfm/contrib/Netxx-0.3.2/")

MAKE_DIRECTORY( ${RESIPROCATE_BINARY_DIR}/tfm/cppunit/)
LINK_DIRECTORIES( ${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/src/cppunit/.libs)
IF(NOT EXISTS ${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/Makefile)
  EXEC_PROGRAM( "/${RESIPROCATE_SOURCE_DIR}/tfm/contrib/cppunit/configure"               "/${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/" )
ENDIF(NOT EXISTS ${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/Makefile)
  
ADD_CUSTOM_TARGET("${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/src/cppunit/.libs" ALL
                    COMMAND "make"
                    WORKING_DIRECTORY "${RESIPROCATE_BINARY_DIR}/tfm/contrib/cppunit/")


ADD_EXECUTABLE(sanityTests sanityTests.cxx)
TARGET_LINK_LIBRARIES(sanityTests tfm repro ${DUM_LIBS} ${RESIPROCATE_LIBS} pcre qdbm Netxx cppunit)

ADD_EXECUTABLE(estacadoTests estacadoTests.cxx)
TARGET_LINK_LIBRARIES(estacadoTests tfm repro ${DUM_LIBS} ${RESIPROCATE_LIBS} pcre qdbm Netxx cppunit)

# Copyright 2007 Estacado Systems
