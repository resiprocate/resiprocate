#if !defined(IChatGatewayCmds_hxx)
#define IChatGatewayCmds_hxx

#include <resip/dum/DumCommand.hxx>

#include "Server.hxx"
#include "B2BSession.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

namespace gateway
{

/**
  The classes defined here are used to pass commands from the
  application thread to the DUM thread (process loop).  
  This ensures thread safety of the external Server methods.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class NotifyIChatCallRequestCmd : public resip::DumCommand
{
   public:  
      NotifyIChatCallRequestCmd(Server& server, const std::string& to, const std::string& from)                       
         : mServer(server), mTo(to), mFrom(from) {}
      virtual void executeCommand()
      {
         mServer.notifyIChatCallRequestImpl(mTo, mFrom);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " NotifyIChatCallRequestCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      std::string mTo;
      std::string mFrom;
};

class NotifyIChatCallCancelledCmd : public resip::DumCommand
{
   public:  
      NotifyIChatCallCancelledCmd(Server& server, const B2BSessionHandle& handle)
         : mServer(server), mHandle(handle) {}
      virtual void executeCommand()
      {
         mServer.notifyIChatCallCancelledImpl(mHandle);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " NotifyIChatCallCancelledCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      B2BSessionHandle mHandle;
};

class NotifyIChatCallProceedingCmd : public resip::DumCommand
{
   public:  
      NotifyIChatCallProceedingCmd(Server& server, const B2BSessionHandle& handle, const std::string& to)                       
         : mServer(server), mHandle(handle), mTo(to) {}
      virtual void executeCommand()
      {
         mServer.notifyIChatCallProceedingImpl(mHandle, mTo);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " NotifyIChatCallProceedingCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      B2BSessionHandle mHandle;
      std::string mTo;
};

class NotifyIChatCallFailedCmd : public resip::DumCommand
{
   public:  
      NotifyIChatCallFailedCmd(Server& server, const B2BSessionHandle& handle, unsigned int statusCode)                       
         : mServer(server), mHandle(handle), mStatusCode(statusCode) {}
      virtual void executeCommand()
      {
         mServer.notifyIChatCallFailedImpl(mHandle, mStatusCode);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " NotifyIChatCallFailedCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      B2BSessionHandle mHandle;
      unsigned int mStatusCode;
};

class ContinueIChatCallCmd : public resip::DumCommand
{
   public:  
      ContinueIChatCallCmd(Server& server, const B2BSessionHandle& handle, const std::string& remoteIPPortListBlob)
         : mServer(server), mHandle(handle), mRemoteIPPortListBlob(remoteIPPortListBlob) {}
      virtual void executeCommand()
      {
         mServer.continueIChatCallImpl(mHandle, mRemoteIPPortListBlob);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " ContinueIChatCallCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      B2BSessionHandle mHandle;
      std::string mRemoteIPPortListBlob;
};

class SipRegisterJabberUserCmd : public resip::DumCommand
{
   public:  
      SipRegisterJabberUserCmd(Server& server, const std::string& jidToRegister)
         : mServer(server), mJidToRegister(jidToRegister) {}
      virtual void executeCommand()
      {
         mServer.sipRegisterJabberUserImpl(mJidToRegister);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " SipRegisterJabberUserCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      std::string mJidToRegister;
};

class SipUnregisterJabberUserCmd : public resip::DumCommand
{
   public:  
      SipUnregisterJabberUserCmd(Server& server, const std::string& jidToUnregister)
         : mServer(server), mJidToUnregister(jidToUnregister) {}
      virtual void executeCommand()
      {
         mServer.sipUnregisterJabberUserImpl(mJidToUnregister);
      }
      resip::Message* clone() const { resip_assert(0); return 0; }
      EncodeStream& encode(EncodeStream& strm) const { strm << " SipUnregisterJabberUserCmd: "; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
   private:
      Server& mServer;
      std::string mJidToUnregister;
};

}

#endif

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

