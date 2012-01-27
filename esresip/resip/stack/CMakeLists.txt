INCLUDE_DIRECTORIES(${RESIPROCATE_SOURCE_DIR} ${RESIPROCATE_SOURCE_DIR}/contrib/ares ${RESIPROCATE_SOURCE_DIR}/repro-deps/log4cplus-1.0.2/include )
SET(RESIP_SOURCES Auth.cxx CSeqCategory.cxx CallId.cxx DateCategory.cxx ExpiresCategory.cxx GenericUri.cxx IntegerCategory.cxx Mime.cxx NameAddr.cxx RequestLine.cxx StatusLine.cxx StringCategory.cxx Token.cxx Via.cxx WarningCategory.cxx Aor.cxx ApiCheck.cxx ApplicationSip.cxx BasicNonceHelper.cxx BranchParameter.cxx Connection.cxx ConnectionBase.cxx ConnectionManager.cxx Contents.cxx ContentsFactoryBase.cxx CpimContents.cxx DataParameter.cxx DeprecatedDialog.cxx DnsInterface.cxx DnsResult.cxx DtlsMessage.cxx DtlsTransport.cxx Embedded.cxx ExtensionParameter.cxx ExtensionHeader.cxx ExistsParameter.cxx ExternalBodyContents.cxx FloatParameter.cxx FlowId.cxx GenericContents.cxx HeaderFieldValue.cxx HeaderFieldValueList.cxx HeaderHash.cxx HeaderTypes.cxx Headers.cxx Helper.cxx IntegerParameter.cxx InternalTransport.cxx LazyParser.cxx Message.cxx MessageWaitingContents.cxx MethodHash.cxx MethodTypes.cxx MsgHeaderScanner.cxx MultipartAlternativeContents.cxx MultipartMixedContents.cxx MultipartRelatedContents.cxx MultipartSignedContents.cxx NonceHelper.cxx OctetContents.cxx Parameter.cxx ParameterHash.cxx ParameterTypes.cxx ParserCategory.cxx ParserContainerBase.cxx Pidf.cxx Pkcs7Contents.cxx Pkcs8Contents.cxx PlainContents.cxx QopParameter.cxx QuotedDataParameter.cxx RAckCategory.cxx Rlmi.cxx RportParameter.cxx SERNonceHelper.cxx SdpContents.cxx Security.cxx SecurityAttributes.cxx SipFrag.cxx SipMessage.cxx SipStack.cxx StackThread.cxx StatisticsManager.cxx StatisticsMessage.cxx Symbols.cxx TcpBaseTransport.cxx TcpConnection.cxx TcpTransport.cxx TimeAccumulate.cxx TimerMessage.cxx TimerQueue.cxx TlsTransport.cxx  Tuple.cxx TransactionController.cxx MessageFilterRule.cxx TransactionUser.cxx TransactionUserMessage.cxx TransactionMap.cxx TransactionState.cxx Transport.cxx TransportFailure.cxx TransportSelector.cxx TuIM.cxx TuSelector.cxx UdpTransport.cxx UnknownParameter.cxx Uri.cxx X509Contents.cxx XMLCursor.cxx KeepAliveMessage.cxx StatelessHandler.cxx InvalidContents.cxx )
SET(TLS_SOURCES TlsConnection.cxx)
ADD_LIBRARY(resip ${RESIP_SOURCES} ${TLS_SOURCES})

INSTALL(TARGETS resip
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

IF(BUILD_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF(BUILD_TESTS)

# Copyright 2007 Estacado Systems
