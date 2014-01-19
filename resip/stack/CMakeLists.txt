# Used for installation
SET(INCLUDES
   AbandonServerTransaction.hxx
   Aor.hxx
   ApiCheck.hxx
   ApiCheckList.hxx
   ApplicationMessage.hxx
   ApplicationSip.hxx
   Auth.hxx
   BasicNonceHelper.hxx
   BranchParameter.hxx
   CallId.hxx
   CancelableTimerQueue.hxx
   CancelClientInviteTransaction.hxx
   Compression.hxx
   Connection.hxx
   ConnectionBase.hxx
   ConnectionManager.hxx
   ConnectionTerminated.hxx
   Contents.hxx
   ContentsFactory.hxx
   ContentsFactoryBase.hxx
   Cookie.hxx
   CpimContents.hxx
   CSeqCategory.hxx
   DataParameter.hxx
   DateCategory.hxx
   DeprecatedDialog.hxx
   DnsInterface.hxx
   DnsResult.hxx
   DnsResultMessage.hxx
   DtlsMessage.hxx
   Embedded.hxx
   EnableFlowTimer.hxx
   EventStackThread.hxx
   ExistsOrDataParameter.hxx
   ExistsParameter.hxx
   ExpiresCategory.hxx
   ExtensionHeader.hxx
   ExtensionParameter.hxx
   ExternalBodyContents.hxx
   FloatParameter.hxx
   GenericContents.hxx
   GenericUri.hxx
   HeaderFieldValue.hxx
   HeaderFieldValueList.hxx
   HeaderHash.hxx
   Headers.hxx
   HeaderTypes.hxx
   Helper.hxx
   IntegerCategory.hxx
   IntegerParameter.hxx
   InternalTransport.hxx
   InteropHelper.hxx
   InterruptableStackThread.hxx
   InvalidContents.hxx
   KeepAliveMessage.hxx
   KeepAlivePong.hxx
   LazyParser.hxx
   MarkListener.hxx
   Message.hxx
   MessageDecorator.hxx
   MessageFilterRule.hxx
   MessageWaitingContents.hxx
   MethodHash.hxx
   MethodTypes.hxx
   Mime.hxx
   MsgHeaderScanner.hxx
   MultipartAlternativeContents.hxx
   MultipartMixedContents.hxx
   MultipartRelatedContents.hxx
   MultipartSignedContents.hxx
   NameAddr.hxx
   NonceHelper.hxx
   OctetContents.hxx
   Parameter.hxx
   ParameterHash.hxx
   ParameterTypeEnums.hxx
   ParameterTypes.hxx
   ParserCategories.hxx
   ParserCategory.hxx
   ParserContainer.hxx
   ParserContainerBase.hxx
   Pidf.hxx
   Pkcs7Contents.hxx
   Pkcs8Contents.hxx
   PlainContents.hxx
   PollStatistics.hxx
   PrivacyCategory.hxx
   QuotedDataParameter.hxx
   QValue.hxx
   QValueParameter.hxx
   RAckCategory.hxx
   RequestLine.hxx
   Rlmi.hxx
   RportParameter.hxx
   SdpContents.hxx
   SecurityAttributes.hxx
   SecurityTypes.hxx
   SendData.hxx
   SERNonceHelper.hxx
   ShutdownMessage.hxx
   SipFrag.hxx
   SipMessage.hxx
   SipStack.hxx
   StackThread.hxx
   StartLine.hxx
   StatelessHandler.hxx
   StatisticsHandler.hxx
   StatisticsManager.hxx
   StatisticsMessage.hxx
   StatusLine.hxx
   StringCategory.hxx
   Symbols.hxx
   TcpBaseTransport.hxx
   TcpConnection.hxx
   TcpTransport.hxx
   TerminateFlow.hxx
   TimeAccumulate.hxx
   TimerMessage.hxx
   TimerQueue.hxx
   Token.hxx
   TransactionController.hxx
   TransactionControllerThread.hxx
   TransactionMap.hxx
   TransactionMessage.hxx
   TransactionState.hxx
   TransactionTerminated.hxx
   TransactionUser.hxx
   TransactionUserMessage.hxx
   Transport.hxx
   TransportFailure.hxx
   TransportSelector.hxx
   TransportSelectorThread.hxx
   TransportThread.hxx
   TuIM.hxx
   Tuple.hxx
   TupleMarkManager.hxx
   TuSelector.hxx
   UdpTransport.hxx
   UInt32Category.hxx
   UInt32Parameter.hxx
   UnknownHeaderType.hxx
   UnknownParameter.hxx
   UnknownParameterType.hxx
   Uri.hxx
   ValueFifo.hxx
   Via.hxx
   WarningCategory.hxx
   WsBaseTransport.hxx
   WsConnection.hxx
   WsConnectionBase.hxx
   WsConnectionValidator.hxx
   WsCookieContext.hxx
   WsCookieContextFactory.hxx
   WsDecorator.hxx
   WsFrameExtractor.hxx
   WsTransport.hxx
   X509Contents.hxx
   ZeroOutStatistics.hxx
)

SET(INCLUDES_SSL
   ssl/DtlsTransport.hxx
   ssl/Security.hxx
   ssl/TlsBaseTransport.hxx
   ssl/TlsConnection.hxx
   ssl/TlsTransport.hxx
   ssl/WssConnection.hxx
   ssl/WssTransport.hxx
)

SET(INCLUDES_SSL_MAC
   ssl/MacSecurity.hxx
)

SET(INCLUDES_SSL_WIN
   ssl/WinSecurity.hxx
)

# Used for compilation
SET(SOURCES
   Aor.cxx
   ApiCheck.cxx
   ApplicationSip.cxx
   Auth.cxx
   BasicNonceHelper.cxx
   BranchParameter.cxx
   CallId.cxx
   Compression.cxx
   Connection.cxx
   ConnectionBase.cxx
   ConnectionManager.cxx
   Contents.cxx
   ContentsFactoryBase.cxx
   Cookie.cxx
   CpimContents.cxx
   CSeqCategory.cxx
   DataParameter.cxx
   DateCategory.cxx
   DeprecatedDialog.cxx
   DnsInterface.cxx
   DnsResult.cxx
   DtlsMessage.cxx
   Embedded.cxx
   EventStackThread.cxx
   ExistsOrDataParameter.cxx
   ExistsParameter.cxx
   ExpiresCategory.cxx
   ExtensionHeader.cxx
   ExtensionParameter.cxx
   ExternalBodyContents.cxx
   FloatParameter.cxx
   GenericContents.cxx
   GenericUri.cxx
   HeaderFieldValue.cxx
   HeaderFieldValueList.cxx
   HeaderHash.cxx
   Headers.cxx
   HeaderTypes.cxx
   Helper.cxx
   IntegerCategory.cxx
   IntegerParameter.cxx
   InternalTransport.cxx
   InteropHelper.cxx
   InterruptableStackThread.cxx
   InvalidContents.cxx
   KeepAliveMessage.cxx
   LazyParser.cxx
   makeCert.cxx
   Message.cxx
   MessageFilterRule.cxx
   MessageWaitingContents.cxx
   MethodHash.cxx
   MethodTypes.cxx
   Mime.cxx
   MsgHeaderScanner.cxx
   MultipartAlternativeContents.cxx
   MultipartMixedContents.cxx
   MultipartRelatedContents.cxx
   MultipartSignedContents.cxx
   NameAddr.cxx
   NonceHelper.cxx
   OctetContents.cxx
   Parameter.cxx
   ParameterHash.cxx
   ParameterTypes.cxx
   ParserCategories.cxx
   ParserCategory.cxx
   ParserContainerBase.cxx
   Pidf.cxx
   Pkcs7Contents.cxx
   Pkcs8Contents.cxx
   PlainContents.cxx
   PrivacyCategory.cxx
   QuotedDataParameter.cxx
   QValue.cxx
   QValueParameter.cxx
   RAckCategory.cxx
   RequestLine.cxx
   Rlmi.cxx
   RportParameter.cxx
   SdpContents.cxx
   SecurityAttributes.cxx
   SERNonceHelper.cxx
   SipFrag.cxx
   SipMessage.cxx
   SipStack.cxx
   StackThread.cxx
   StatelessHandler.cxx
   StatisticsHandler.cxx
   StatisticsManager.cxx
   StatisticsMessage.cxx
   StatusLine.cxx
   StringCategory.cxx
   Symbols.cxx
   TcpBaseTransport.cxx
   TcpConnection.cxx
   TcpTransport.cxx
   TimeAccumulate.cxx
   TimerMessage.cxx
   TimerQueue.cxx
   Token.cxx
   TransactionController.cxx
   TransactionMap.cxx
   TransactionState.cxx
   TransactionUser.cxx
   TransactionUserMessage.cxx
   Transport.cxx
   TransportFailure.cxx
   TransportSelector.cxx
   TransportThread.cxx
   TuIM.cxx
   Tuple.cxx
   TupleMarkManager.cxx
   TuSelector.cxx
   UdpTransport.cxx
   UInt32Category.cxx
   UInt32Parameter.cxx
   UnknownParameter.cxx
   Uri.cxx
   Via.cxx
   WarningCategory.cxx
   WsBaseTransport.cxx
   WsConnection.cxx
   WsConnectionBase.cxx
   WsCookieContext.cxx
   WsDecorator.cxx
   WsFrameExtractor.cxx
   WsTransport.cxx
   X509Contents.cxx
)

SET(SOURCES_SSL
   ssl/DtlsTransport.cxx
   ssl/Security.cxx
   ssl/TlsBaseTransport.cxx
   ssl/TlsConnection.cxx
   ssl/TlsTransport.cxx
   ssl/WssConnection.cxx
   ssl/WssTransport.cxx
)

SET(SOURCES_SSL_MAC
   ssl/MacSecurity.cxx
)

SET(SOURCES_SSL_WIN
   ssl/WinSecurity.cxx
)

# If compiling with OpenSSL support, add extra SSL related source and header
# files
IF(${OPENSSL_FOUND})
   LIST(APPEND SOURCES ${SOURCES_SSL}) 
   LIST(APPEND INCLUDES ${INCLUDES_SSL}) 

   IF(CMAKE_HOST_WIN32)
      LIST(APPEND SOURCES ${SOURCES_SSL_WIN}) 
      LIST(APPEND INCLUDES_SSL ${INCLUDES_SSL_WIN})
   ELSEIF(CMAKE_HOST_APPLE)
      LIST(APPEND SOURCES ${SOURCES_SSL_MAC}) 
      LIST(APPEND INCLUDES_SSL ${INCLUDES_SSL_MAC})
   ENDIF()
   
   # Since some ssl files are under the ssl folder they require their own
   # INSTALL command
   INSTALL(FILES ${INCLUDES_SSL} DESTINATION include/resip/stack/ssl)
ENDIF()

INCLUDE_DIRECTORIES(../..)

SET(TARGET_LIBS ${OPENSSL_LIBRARIES} rutil ${ARES_LIBRARIES})

IF(CMAKE_HOST_WIN32)
   # For whatever historical reason, on Windows the name of the library is not
   # resip but resiprocate.
   SET(TARGET_NAME resiprocate)

   # Static library on Windows
   SET(TARGET_TYPE "")

   # Also add Windows-related source and header files
   LIST(APPEND INCLUDES ${INCLUDES_SSL_WIN})

   LIST(APPEND TARGET_LIBS winmm)
ELSE()
   SET(TARGET_NAME resip)

   # Shared library on Unix
   SET(TARGET_TYPE SHARED)
   
   IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      LIST(APPEND TARGET_LIBS rt)
   ENDIF()
ENDIF()

ADD_LIBRARY(${TARGET_NAME} ${TARGET_TYPE} ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${TARGET_LIBS})
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES FOLDER resip)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)

INSTALL(FILES ${INCLUDES} DESTINATION include/resip/stack)

# TODO Is makeCert a standalone app?

ADD_SUBDIRECTORY(test)
