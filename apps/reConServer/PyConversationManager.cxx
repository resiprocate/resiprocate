
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>

#include "PyConversationManager.hxx"
#include "MyUserAgent.hxx"

#include <rutil/Logger.hxx>
#include "AppSubsystem.hxx"

#include <resip/recon/LocalParticipant.hxx>
#include <resip/recon/RemoteParticipant.hxx>
#include <resip/recon/Conversation.hxx>
#ifdef USE_GSTREAMER
#include <resip/recon/GstRemoteParticipant.hxx>
#endif
#ifdef USE_KURENTO
#include <media/kurento/Object.hxx>
#include <resip/recon/KurentoRemoteParticipant.hxx>
#endif

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace std;
using namespace resip;
using namespace recon;
using namespace reconserver;

PyConversationManager::PyConversationManager(const ReConServerConfig& config, bool localAudioEnabled, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled)
      : MyConversationManager(config, localAudioEnabled, defaultSampleRate, maxSampleRate, autoAnswerEnabled),
        PyExtensionBase("resip")
{
   Data pyPath(config.getConfigData("PyScriptPath", "", true));
   mScriptName = config.getConfigData("PyScript", "conversationManager", true);
   if(pyPath.empty())
   {
      ErrLog(<<"PyScriptPath not specified in config, aborting");
      throw ConfigParse::Exception("PyScriptPath not specified in config", __FILE__, __LINE__);
   }
   if(mScriptName.empty())
   {
      ErrLog(<<"PyScript not specified in config, aborting");
      throw ConfigParse::Exception("PyScript not specified in config", __FILE__, __LINE__);
   }

   if(!PyExtensionBase::init(pyPath))
   {
      ErrLog(<<"PyExtensionBase::init failed");
      throw std::runtime_error("PyExtensionBase::init failed");
   }

   // FIXME - work queue
   //int numPyWorkerThreads = config.getConfigInt("PyNumWorkerThreads", 2);
   //std::unique_ptr<Worker> worker(new PyWorker(*this, mAction));
   //mDispatcher.reset(new Dispatcher(std::move(worker), &sipStack, numPyWorkerThreads));
}

void
PyConversationManager::startup()
{
   // reConServer invokes this method after both ConversationManager and UserAgent constructed

   MyConversationManager::startup();
}

bool
PyConversationManager::onStartup()
{
   // onStartup is called from PyExtensionBase::init( )
   // while it still has the lock

   // FIXME - work queue
   std::unique_ptr<Py::Module> pyModule = loadModule(mScriptName);
   if(!pyModule)
   {
      ErrLog(<<"loadModule failed");
      return false;
   }

   // Using the line
   //     import resip.recon
   // in a script fails with an error "No module named 'resip'".
   // Therefore, we force it to be included in the dictionary for the
   // script we are invoking.
   //importModule(pyModule->ptr(), "resip.recon");
   //Py::Module m = Py::ExtensionModule<PyConversationManager>::module();
   //addSubModule(Py::ExtensionModule<PyConversationManager>::module(), "recon");

   if(pyModule->getDict().hasKey("on_load"))
   {
      DebugLog(<< "found on_load method, trying to invoke it...");
      try
      {
         StackLog(<< "invoking on_load");
         pyModule->callMemberFunction("on_load");
      }
      catch (const Py::Exception& ex)
      {
         ErrLog(<< "call to on_load method failed: " << Py::value(ex));
         StackLog(<< Py::trace(ex));
         return false;
      }
   }

   mPyModule = std::move(pyModule);

   DebugLog(<< "creating new PyThreadState");
   mPyUser = createPyExternalUser();

   return true;
}

void
PyConversationManager::onParticipantTerminated(ParticipantHandle partHandle, unsigned int statusCode)
{
   MyConversationManager::onParticipantTerminated(partHandle, statusCode);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   args.append(Py::Long(static_cast<unsigned long>(statusCode)));
   Py::Object response;
   doPythonCall("on_participant_terminated", args, response);
}

void
PyConversationManager::onParticipantDestroyed(ParticipantHandle partHandle)
{
   MyConversationManager::onParticipantDestroyed(partHandle);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   Py::Object response;
   doPythonCall("on_participant_destroyed", args, response);
}

void
PyConversationManager::onParticipantProceeding(ParticipantHandle partHandle, const resip::SipMessage& msg)
{
   MyConversationManager::onParticipantProceeding(partHandle, msg);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   // FIXME - more args
   Py::Object response;
   doPythonCall("on_participant_proceeding", args, response);
}

void
PyConversationManager::onParticipantAlerting(ParticipantHandle partHandle, const resip::SipMessage& msg)
{
   MyConversationManager::onParticipantAlerting(partHandle, msg);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   // FIXME - more args
   Py::Object response;
   doPythonCall("on_participant_alerting", args, response);
}

void
PyConversationManager::onParticipantConnected(ParticipantHandle partHandle, const resip::SipMessage& msg)
{
   MyConversationManager::onParticipantConnected(partHandle, msg);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   // FIXME - more args
   Py::Object response;
   doPythonCall("on_participant_connected", args, response);
}

void
PyConversationManager::onParticipantConnectedConfirmed(ParticipantHandle partHandle, const resip::SipMessage& msg)
{
   MyConversationManager::onParticipantConnectedConfirmed(partHandle, msg);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   // FIXME - more args
   Py::Object response;
   doPythonCall("on_participant_connected_confirmed", args, response);
}

void
PyConversationManager::onDtmfEvent(ParticipantHandle partHandle, int dtmf, int duration, bool up)
{
   MyConversationManager::onDtmfEvent(partHandle, dtmf, duration, up);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   args.append(Py::Long(dtmf));
   args.append(Py::Long(duration));
   args.append(pyTrueFalse(up));
   Py::Object response;
   doPythonCall("on_dtmf_event", args, response);
}

void
PyConversationManager::onIncomingParticipant(ParticipantHandle partHandle, const SipMessage& msg, bool autoAnswer, ConversationProfile& conversationProfile)
{
   MyConversationManager::onIncomingParticipant(partHandle, msg, autoAnswer, conversationProfile);

   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   Py::List args;
   args.append(pyPartHandle(partHandle));
   args.append(Py::String(msg.header(resip::h_RequestLine).uri().toString().c_str()));
   Py::Dict headers;
   headers["From"] = Py::String(msg.header(resip::h_From).uri().toString().c_str());
   headers["To"] = Py::String(msg.header(resip::h_To).uri().toString().c_str());
   args.append(headers);
   args.append(pyTrueFalse(autoAnswer));
   Py::Object response;
   doPythonCall("on_incoming_participant", args, response);
}

void
PyConversationManager::initMethods()
{
   PyExtensionBase::initMethods();

   // FIXME - implement these in a standalone Python module
   //         for the resip.recon API

   // Py::ExtensionModule expects the callback functions to be
   // of type T::* and not a subclass of T so we must cast them here

   add_varargs_method("get_room", (method_varargs_function_t)&PyConversationManager::pyGetRoom, "get_room(roomName) = get conversation for room name");
   add_varargs_method("create_conversation", (method_varargs_function_t)&PyConversationManager::pyCreateConversation, "create_conversation() = create a new conversation");
   add_varargs_method("destroy_conversation", (method_varargs_function_t)&PyConversationManager::pyDestroyConversation, "destroy_conversation() = destroy a conversation");
   add_varargs_method("create_remote_participant", (method_varargs_function_t)&PyConversationManager::pyCreateRemoteParticipant, "create_remote_participant(convHandle, dest) = create outgoing call to participant");
   add_varargs_method("create_media_resource_participant", (method_varargs_function_t)&PyConversationManager::pyCreateMediaResourceParticipant, "create_media_resource_participant(convHandle, mediaURL) = create a media resource participant");
   add_varargs_method("destroy_participant", (method_varargs_function_t)&PyConversationManager::pyDestroyParticipant, "destroy_participant() = destroy a participant");
   add_varargs_method("add_participant", (method_varargs_function_t)&PyConversationManager::pyAddParticipant, "add_participant(convHandle, partHandle = add participant to conversation");
   add_varargs_method("remove_participant", (method_varargs_function_t)&PyConversationManager::pyRemoveParticipant, "remove_participant(convHandle, partHandle) = remove a participant");
   add_varargs_method("move_participant", (method_varargs_function_t)&PyConversationManager::pyMoveParticipant, "move_participant(partHandle, sourceConvHandle, destConvHandle) = move a participant");
   add_varargs_method("alert_participant", (method_varargs_function_t)&PyConversationManager::pyAlertParticipant, "alert_participant(partHandle) - indicate ringing");
   add_varargs_method("answer_participant", (method_varargs_function_t)&PyConversationManager::pyAnswerParticipant, "answer_participant(partHandle) = answer participant");
   add_varargs_method("reject_participant", (method_varargs_function_t)&PyConversationManager::pyRejectParticipant, "reject_participant(partHandle) - reject an incoming participant");
}

Py::Object
PyConversationManager::pyGetRoom(const Py::Tuple& args)
{
   if(!checkPyArgs("get_room", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::String& roomName(*it++);
   ConversationHandle convHandle = getRoom(roomName.as_std_string().c_str());
   return pyConvHandle(convHandle);
}

Py::Object
PyConversationManager::pyCreateConversation(const Py::Tuple& args)
{
   if(!checkPyArgs("create_conversation", args, 0, 0))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   // FIXME - argument missing
   //Py::Tuple::const_iterator it = args.begin();
   //const Py::Object& autoHoldMode(*it++);
   createConversation();
   return Py::None();
}

Py::Object
PyConversationManager::pyDestroyConversation(const Py::Tuple& args)
{
   if(!checkPyArgs("destroy_conversation", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& convHandle(*it++);
   destroyConversation(cConvHandle(convHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyCreateRemoteParticipant(const Py::Tuple& args)
{
   // FIXME - missing args
   if(!checkPyArgs("create_remote_participant", args, 2, 2))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& convHandle(*it++);
   const Py::String& dest(*it++);
   NameAddr _dest(dest.as_std_string().c_str());
   createRemoteParticipant(cConvHandle(convHandle), _dest);
   return Py::None();
}

Py::Object
PyConversationManager::pyCreateMediaResourceParticipant(const Py::Tuple& args)
{
   // FIXME - missing args
   if(!checkPyArgs("create_media_resource_participant", args, 2, 2))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& convHandle(*it++);
   const Py::String& mediaUrl(*it++);
   Uri _mediaUrl(mediaUrl.as_std_string().c_str());
   createMediaResourceParticipant(cConvHandle(convHandle), _mediaUrl);
   return Py::None();
}

Py::Object
PyConversationManager::pyDestroyParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("destroy_participant", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& partHandle(*it++);
   destroyParticipant(cPartHandle(partHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyAddParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("add_participant", args, 2, 2))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& convHandle(*it++);
   const Py::Object& partHandle(*it++);
   addParticipant(cConvHandle(convHandle), cPartHandle(partHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyRemoveParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("remove_participant", args, 2, 2))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& convHandle(*it++);
   const Py::Object& partHandle(*it++);
   removeParticipant(cConvHandle(convHandle), cPartHandle(partHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyMoveParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("move_participant", args, 3, 3))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& partHandle(*it++);
   const Py::Object& sourceConvHandle(*it++);
   const Py::Object& destConvHandle(*it++);
   moveParticipant(cPartHandle(partHandle), cConvHandle(sourceConvHandle), cConvHandle(destConvHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyAlertParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("alert_participant", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& partHandle(*it++);
   alertParticipant(cPartHandle(partHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyAnswerParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("answer_participant", args, 1, 1))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& partHandle(*it++);
   answerParticipant(cPartHandle(partHandle));
   return Py::None();
}

Py::Object
PyConversationManager::pyRejectParticipant(const Py::Tuple& args)
{
   if(!checkPyArgs("reject_participant", args, 2, 2))
   {
      throw Py::TypeError("insufficient number of arguments");
   }
   Py::Tuple::const_iterator it = args.begin();
   const Py::Object& partHandle(*it++);
   const Py::Long rejectCode(*it++);
   rejectParticipant(cPartHandle(partHandle), (unsigned long)rejectCode);
   return Py::None();
}

bool
PyConversationManager::doPythonCall(const char* method, Py::List& args, Py::Object& response)
{
   Py::Callable action;
   try
   {
      action = mPyModule->getAttr(method);
   }
   catch (const Py::Exception& ex)
   {
      ErrLog(<< "PyScript action method not found: " << method);
      return false;
   }
   Py::Tuple _args(args);
   try
   {
      StackLog(<< "invoking action");
      response = action.apply(_args);
      return true;
   }
   catch (const Py::Exception& ex)
   {
      WarningLog(<< "PyScript action failed: " << Py::value(ex));
      WarningLog(<< Py::trace(ex));
      return false;
   }
}

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

