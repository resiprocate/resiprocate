# Used for installation
SET(INCLUDES
   AppDialog.hxx
   AppDialogSet.hxx
   AppDialogSetFactory.hxx
   BaseCreator.hxx
   BaseSubscription.hxx
   BaseUsage.hxx
   CertMessage.hxx
   ChallengeInfo.hxx
   ClientAuthExtension.hxx
   ClientAuthManager.hxx
   ClientInviteSession.hxx
   ClientOutOfDialogReq.hxx
   ClientPagerMessage.hxx
   ClientPublication.hxx
   ClientRegistration.hxx
   ClientSubscription.hxx
   ClientSubscriptionFunctor.hxx
   ContactInstanceRecord.hxx
   DefaultServerReferHandler.hxx
   DestroyUsage.hxx
   Dialog.hxx
   DialogEventHandler.hxx
   DialogEventInfo.hxx
   DialogEventStateManager.hxx
   DialogId.hxx
   DialogSet.hxx
   DialogSetHandler.hxx
   DialogSetId.hxx
   DialogUsage.hxx
   DialogUsageManager.hxx
   DumCommand.hxx
   DumDecrypted.hxx
   DumException.hxx
   DumFeature.hxx
   DumFeatureChain.hxx
   DumFeatureMessage.hxx
   DumHelper.hxx
   DumProcessHandler.hxx
   DumShutdownHandler.hxx
   DumThread.hxx
   DumTimeout.hxx
   EncryptionRequest.hxx
   EventDispatcher.hxx
   ExternalMessageBase.hxx
   ExternalMessageHandler.hxx
   ExternalTimer.hxx
   Handle.hxx
   HandleException.hxx
   HandleManager.hxx
   Handled.hxx
   Handles.hxx
   HttpGetMessage.hxx
   HttpProvider.hxx
   IdentityHandler.hxx
   InMemoryRegistrationDatabase.hxx
   InMemorySyncRegDb.hxx
   InviteDialogs.hxx
   InviteSession.hxx
   InviteSessionCreator.hxx
   InviteSessionHandler.hxx
   KeepAliveManager.hxx
   KeepAliveTimeout.hxx
   MasterProfile.hxx
   MergedRequestKey.hxx
   MergedRequestRemovalCommand.hxx
   NetworkAssociation.hxx
   NonDialogUsage.hxx
   OutOfDialogHandler.hxx
   OutOfDialogReqCreator.hxx
   OutgoingEvent.hxx
   PagerMessageCreator.hxx
   PagerMessageHandler.hxx
   Postable.hxx
   Profile.hxx
   PublicationCreator.hxx
   PublicationHandler.hxx
   RADIUSServerAuthManager.hxx
   RedirectHandler.hxx
   RedirectManager.hxx
   RefCountedDestroyer.hxx
   RegistrationCreator.hxx
   RegistrationHandler.hxx
   RegistrationPersistenceManager.hxx
   RemoteCertStore.hxx
   RequestValidationHandler.hxx
   ServerAuthManager.hxx
   ServerInviteSession.hxx
   ServerOutOfDialogReq.hxx
   ServerPagerMessage.hxx
   ServerPublication.hxx
   ServerRegistration.hxx
   ServerSubscription.hxx
   ServerSubscriptionFunctor.hxx
   SubscriptionCreator.hxx
   SubscriptionHandler.hxx
   SubscriptionPersistenceManager.hxx
   SubscriptionState.hxx
   TargetCommand.hxx
   TlsPeerAuthManager.hxx
   UsageUseException.hxx
   UserAuthInfo.hxx
   UserProfile.hxx
   WsCookieAuthManager.hxx
)

SET(INCLUDES_SSL
   ssl/EncryptionManager.hxx
)

# Used for compilation
SET(SOURCES
   AppDialog.cxx
   AppDialogSet.cxx
   AppDialogSetFactory.cxx
   BaseCreator.cxx
   BaseSubscription.cxx
   BaseUsage.cxx
   CertMessage.cxx
   ChallengeInfo.cxx
   ClientAuthExtension.cxx
   ClientAuthManager.cxx
   ClientInviteSession.cxx
   ClientOutOfDialogReq.cxx
   ClientPagerMessage.cxx
   ClientPublication.cxx
   ClientRegistration.cxx
   ClientSubscription.cxx
   ContactInstanceRecord.cxx
   DefaultServerReferHandler.cxx
   DestroyUsage.cxx
   Dialog.cxx
   DialogEventInfo.cxx
   DialogEventStateManager.cxx
   DialogId.cxx
   DialogSet.cxx
   DialogSetId.cxx
   DialogUsage.cxx
   DialogUsageManager.cxx
   DumDecrypted.cxx
   DumFeature.cxx
   DumFeatureChain.cxx
   DumFeatureMessage.cxx
   DumHelper.cxx
   DumProcessHandler.cxx
   DumThread.cxx
   DumTimeout.cxx
   EncryptionRequest.cxx
   Handle.cxx
   HandleException.cxx
   HandleManager.cxx
   Handled.cxx
   HttpGetMessage.cxx
   HttpProvider.cxx
   IdentityHandler.cxx
   InMemoryRegistrationDatabase.cxx
   InMemorySyncRegDb.cxx
   InviteSession.cxx
   InviteSessionCreator.cxx
   InviteSessionHandler.cxx
   KeepAliveManager.cxx
   KeepAliveTimeout.cxx
   MasterProfile.cxx
   MergedRequestKey.cxx
   MergedRequestRemovalCommand.cxx
   NetworkAssociation.cxx
   NonDialogUsage.cxx
   OutOfDialogReqCreator.cxx
   OutgoingEvent.cxx
   PagerMessageCreator.cxx
   Profile.cxx
   PublicationCreator.cxx
   RADIUSServerAuthManager.cxx
   RedirectManager.cxx
   RegistrationCreator.cxx
   RegistrationHandler.cxx
   ServerAuthManager.cxx
   ServerInviteSession.cxx
   ServerOutOfDialogReq.cxx
   ServerPagerMessage.cxx
   ServerPublication.cxx
   ServerRegistration.cxx
   ServerSubscription.cxx
   SubscriptionCreator.cxx
   SubscriptionHandler.cxx
   SubscriptionState.cxx
   TargetCommand.cxx
   TlsPeerAuthManager.cxx
   UserAuthInfo.cxx
   UserProfile.cxx
   WsCookieAuthManager.cxx
)

SET(SOURCES_SSL
   ssl/EncryptionManager.cxx
)

# If compiling with OpenSSL support, add extra SSL related source and header
# files
IF(${OPENSSL_FOUND})
   LIST(APPEND SOURCES ${SOURCES_SSL}) 
   LIST(APPEND INCLUDES ${INCLUDES_SSL}) 
   
   # Since some ssl files are under the ssl folder they require their own
   # INSTALL command
   INSTALL(FILES ${INCLUDES_SSL} DESTINATION include/resip/dum/ssl)
ENDIF()

INCLUDE_DIRECTORIES(../..)

SET(TARGET_LIBS ${OPENSSL_LIBRARIES} rutil ${ARES_LIBRARIES})

IF(CMAKE_HOST_WIN32)
   # Static library on Windows
   SET(TARGET_TYPE "")

   LIST(APPEND TARGET_LIBS resiprocate)

   # TODO Is this really needed?
   LIST(APPEND TARGET_LIBS winmm)
ELSE()
   # Shared library on Unix
   SET(TARGET_TYPE SHARED)

   LIST(APPEND TARGET_LIBS resip)   

   # TODO Is this really needed?
   IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      LIST(APPEND TARGET_LIBS rt)
   ENDIF()
ENDIF()

ADD_LIBRARY(dum ${TARGET_TYPE} ${SOURCES} ${INCLUDES})
TARGET_LINK_LIBRARIES(dum ${TARGET_LIBS})
SET_TARGET_PROPERTIES(dum PROPERTIES FOLDER resip)

INSTALL(TARGETS dum DESTINATION lib)

INSTALL(FILES ${INCLUDES} DESTINATION include/resip/dum)

ADD_SUBDIRECTORY(test)
