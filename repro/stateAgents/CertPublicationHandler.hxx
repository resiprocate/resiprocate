#if !defined(Repro_CertPublicationHandler_hxx)
#define Repro_CertPublicationHandler_hxx

#include "resip/dum/PublicationHandler.hxx"

namespace resip
{
class Security;
class SipMessage;
class SecurityAttributes;
class Data;
class Contents;
}

namespace repro
{

class CertPublicationHandler : public resip::ServerPublicationHandler
{
   public:
      CertPublicationHandler(resip::Security& security);
      virtual void onInitial(resip::ServerPublicationHandle h, 
                             const resip::Data& etag, 
                             const resip::SipMessage& pub, 
                             const resip::Contents* contents,
                             const resip::SecurityAttributes* attrs, 
                             UInt32 expires);
      virtual void onExpired(resip::ServerPublicationHandle h, const resip::Data& etag);
      virtual void onRefresh(resip::ServerPublicationHandle, 
                             const resip::Data& etag, 
                             const resip::SipMessage& pub, 
                             const resip::Contents* contents,
                             const resip::SecurityAttributes* attrs,
                             UInt32 expires);
      virtual void onUpdate(resip::ServerPublicationHandle h, 
                            const resip::Data& etag, 
                            const resip::SipMessage& pub, 
                            const resip::Contents* contents,
                            const resip::SecurityAttributes* attrs,
                            UInt32 expires);
      virtual void onRemoved(resip::ServerPublicationHandle h, const resip::Data& etag, const resip::SipMessage& pub, UInt32 expires);
      
   private:
      void add(resip::ServerPublicationHandle h, const resip::Contents* contents);
      
      resip::Security& mSecurity;
};
 
}

#endif 

/* ====================================================================
*
* Copyright (c) 2015.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*/
/*
* vi: set shiftwidth=3 expandtab:
*/

