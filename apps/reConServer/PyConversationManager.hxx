#ifndef PYCONVERSATIONMANAGER_HXX
#define PYCONVERSATIONMANAGER_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/Data.hxx>
#include <rutil/PyExtensionBase.hxx>
#include <resip/stack/Dispatcher.hxx>

#include "MyConversationManager.hxx"
#include "reConServerConfig.hxx"

namespace reconserver
{

class PyConversationManager : public reconserver::MyConversationManager,
                              public resip::PyExtensionBase
{
public:

   PyConversationManager(const ReConServerConfig& config, bool localAudioEnabled, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled);
   virtual ~PyConversationManager() {};

   virtual void startup() override;

   //virtual void onConversationDestroyed(recon::ConversationHandle convHandle) override;
   virtual void onParticipantDestroyed(recon::ParticipantHandle partHandle) override;
   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up) override;
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile) override;
   //virtual void onRequestOutgoingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, recon::ConversationProfile& conversationProfile) override;
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   //virtual void onRelatedConversation(recon::ConversationHandle relatedConvHandle, recon::ParticipantHandle relatedPartHandle,
   //                                   recon::ConversationHandle origConvHandle, recon::ParticipantHandle origPartHandle) override;
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantConnectedConfirmed(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   //virtual void onParticipantRedirectSuccess(recon::ParticipantHandle partHandle) override;
   //virtual void onParticipantRedirectFailure(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   //virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held) override;
   //virtual void displayInfo();

protected:
   virtual void initMethods() override;
   Py::Object pyGetRoom(const Py::Tuple& args);
   Py::Object pyCreateConversation(const Py::Tuple& args);
   Py::Object pyDestroyConversation(const Py::Tuple& args);
   Py::Object pyCreateRemoteParticipant(const Py::Tuple& args);
   Py::Object pyCreateMediaResourceParticipant(const Py::Tuple& args);
   Py::Object pyDestroyParticipant(const Py::Tuple& args);
   Py::Object pyAddParticipant(const Py::Tuple& args);
   Py::Object pyRemoveParticipant(const Py::Tuple& args);
   Py::Object pyMoveParticipant(const Py::Tuple& args);
   Py::Object pyAlertParticipant(const Py::Tuple& args);
   Py::Object pyAnswerParticipant(const Py::Tuple &args);
   Py::Object pyRejectParticipant(const Py::Tuple& args);

private:
   virtual bool onStartup() override;
   bool doPythonCall(const char* method, Py::List& args, Py::Object& response);
   Py::Object pyPartHandle(recon::ParticipantHandle p) { return Py::Long((long unsigned int)p);}; // FIXME
   recon::ParticipantHandle cPartHandle(const Py::Object& l) { return (long)static_cast<const Py::Long&>(l); }; // FIXME
   Py::Object pyConvHandle(recon::ConversationHandle c) { return Py::Long((long unsigned int)c);}; // FIXME
   recon::ConversationHandle cConvHandle(const Py::Object& l) { return (long)static_cast<const Py::Long&>(l); }; // FIXME
   Py::Object pyTrueFalse(bool v) { return v ? Py::True() : Py::False(); };
   resip::Data mScriptName;
   std::unique_ptr<Py::Module> mPyModule;
   std::unique_ptr<resip::PyExternalUser> mPyUser;
   // FIXME - work queue
   //std::unique_ptr<resip::Dispatcher> mDispatcher;

};

}

#endif

/* ====================================================================

 Copyright (c) 2022, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2013-2022, Daniel Pocock https://danielpocock.com
 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

