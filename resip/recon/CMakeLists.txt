
set(INCLUDES
   ReconSubsystem.hxx
   LocalParticipant.hxx
   MediaResourceCache.hxx
   RelatedConversationSet.hxx
   UserAgentServerAuthManager.hxx
   ConversationManagerCmds.hxx
   IMParticipantBase.hxx
   UserAgentMasterProfile.hxx
   RemoteParticipantDialogSet.hxx
   SipXHelper.hxx
   Conversation.hxx
   UserAgentDialogSetFactory.hxx
   UserAgentCmds.hxx
   UserAgentRegistration.hxx
   MediaEvent.hxx
   MediaStackAdapter.hxx
   DefaultDialogSet.hxx
   Participant.hxx
   UserAgentClientSubscription.hxx
   UserAgentClientPublication.hxx
   UserAgent.hxx
   ConversationParticipantAssignment.hxx
   RemoteParticipant.hxx
   RemoteIMPagerParticipant.hxx
   RemoteIMSessionParticipant.hxx
   RemoteIMSessionParticipantDialogSet.hxx
   MediaResourceParticipant.hxx
   BridgeMixer.hxx
   HandleTypes.hxx
   MediaStreamEvent.hxx
   ConversationProfile.hxx
   ConversationManager.hxx
   DtmfEvent.hxx
   sdp/SdpMediaLine.hxx
   sdp/SdpCodec.hxx
   sdp/SdpHelperResip.hxx
   sdp/SdpCandidatePair.hxx
   sdp/SdpCandidate.hxx
   sdp/Sdp.hxx
   SipXMediaStackAdapter.hxx
   SipXMediaInterface.hxx
   SipXConversation.hxx
   SipXBridgeMixer.hxx
   SipXParticipant.hxx
   SipXLocalParticipant.hxx
   SipXMediaResourceParticipant.hxx
   SipXRemoteParticipant.hxx
   SipXRemoteParticipantDialogSet.hxx
   FlowManagerSipXSocket.hxx
   KurentoBridgeMixer.hxx
   KurentoConversation.hxx
   KurentoMediaStackAdapter.hxx
   KurentoParticipant.hxx
   KurentoRemoteParticipantDialogSet.hxx
   KurentoRemoteParticipant.hxx
   GstBridgeMixer.hxx
   GstConversation.hxx
   GstMediaStackAdapter.hxx
   GstParticipant.hxx
   GstRemoteParticipantDialogSet.hxx
   GstRemoteParticipant.hxx
   LibWebRTCBridgeMixer.hxx
   LibWebRTCConversation.hxx
   LibWebRTCMediaStackAdapter.hxx
   LibWebRTCParticipant.hxx
   LibWebRTCRemoteParticipantDialogSet.hxx
   LibWebRTCRemoteParticipant.hxx
)

add_library(recon
   BridgeMixer.cxx
   Conversation.cxx
   ConversationManager.cxx
   ConversationParticipantAssignment.cxx
   ConversationProfile.cxx
   DefaultDialogSet.cxx
   DtmfEvent.cxx
   IMParticipantBase.cxx
   LocalParticipant.cxx
   MediaEvent.cxx
   MediaStackAdapter.cxx
   MediaResourceParticipant.cxx
   MediaResourceCache.cxx
   MediaStreamEvent.cxx
   Participant.cxx
   ReconSubsystem.cxx
   RelatedConversationSet.cxx
   RemoteParticipant.cxx
   RemoteParticipantDialogSet.cxx
   RemoteIMPagerParticipant.cxx
   RemoteIMSessionParticipant.cxx
   RemoteIMSessionParticipantDialogSet.cxx
   UserAgent.cxx
   UserAgentClientSubscription.cxx
   UserAgentClientPublication.cxx
   UserAgentDialogSetFactory.cxx
   UserAgentMasterProfile.cxx
   UserAgentRegistration.cxx
   UserAgentServerAuthManager.cxx
   ${INCLUDES}
)

target_add_conditional_sources(recon USE_SIPXTAPI
   FlowManagerSipXSocket.cxx
   SipXMediaStackAdapter.cxx
   SipXMediaInterface.cxx
   SipXConversation.cxx
   SipXBridgeMixer.cxx
   SipXParticipant.cxx
   SipXLocalParticipant.cxx
   SipXMediaResourceParticipant.cxx
   SipXRemoteParticipant.cxx
   SipXRemoteParticipantDialogSet.cxx
   SipXHelper.cxx
   sdp/Sdp.cxx
   sdp/SdpCandidate.cxx
   sdp/SdpCandidatePair.cxx
   sdp/SdpCodec.cxx
   sdp/SdpHelperResip.cxx
   sdp/SdpMediaLine.cxx
)

target_add_conditional_sources(recon USE_KURENTO
   KurentoBridgeMixer.cxx
   KurentoConversation.cxx
   KurentoMediaStackAdapter.cxx
   KurentoParticipant.cxx
   KurentoRemoteParticipant.cxx
   KurentoRemoteParticipantDialogSet.cxx
)

target_add_conditional_sources(recon USE_GSTREAMER
   GstBridgeMixer.cxx
   GstConversation.cxx
   GstMediaStackAdapter.cxx
   GstParticipant.cxx
   GstRemoteParticipant.cxx
   GstRemoteParticipantDialogSet.cxx
)

target_add_conditional_sources(recon USE_LIBWEBRTC
   LibWebRTCBridgeMixer.cxx
   LibWebRTCConversation.cxx
   LibWebRTCMediaStackAdapter.cxx
   LibWebRTCParticipant.cxx
   LibWebRTCRemoteParticipant.cxx
   LibWebRTCRemoteParticipantDialogSet.cxx
)

target_include_directories(recon PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)
target_include_directories(recon PUBLIC ${SIPXTAPI_INCLUDE_DIRS})
target_link_libraries(recon PUBLIC dum)
target_link_libraries(recon PUBLIC resipmedia)
target_link_libraries(recon PUBLIC reflow)
if(USE_SIPXTAPI)
   if(WIN32)
      add_dependencies(recon sipXtapi-pcre sipXportLib sipXsdpLib sipXmediaLib sipXmediaAdapterLib)
   else()
      target_link_libraries(recon PUBLIC ${SIPXTAPI_LIBRARIES})
   endif()
endif()
if(USE_KURENTO)
   target_link_libraries(recon PUBLIC kurentoclient)
endif()
if(USE_GSTREAMER)
   target_link_libraries(recon PUBLIC gstreamerutils)
endif()
version_libname(recon)

install(TARGETS recon DESTINATION ${CMAKE_INSTALL_LIBDIR})
install_and_preserve_hierarchy(${CMAKE_INSTALL_INCLUDEDIR}/recon ${INCLUDES})

# FIXME - revise the code in these directories to eliminate
# dependency on sipXtapi
if(USE_SIPXTAPI)
   add_subdirectory(MOHParkServer)
   if(BUILD_TESTING)
       add_subdirectory(test)
   endif()
endif()

set_target_properties(recon PROPERTIES FOLDER recon)


