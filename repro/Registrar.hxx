#if !defined(RESIP_REGISTRAR_HXX)
#define RESIP_REGISTRAR_HXX 

#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InMemoryRegistrationDatabase.hxx"
#include "resip/dum/MasterProfile.hxx"

namespace repro
{
class Proxy;

class RegistrarHandler
{
   public:
      virtual ~RegistrarHandler() {}

      /// Note: These very closely mimic the ServerRegistrationHandler callbacks

      /// For all of the below callbacks - return true if no response was generated and processing
      /// should continue.  If all handlers return true, then the Registrar will accept the request.

      /// Called when registration is refreshed
      virtual bool onRefresh(resip::ServerRegistrationHandle, const resip::SipMessage& reg)=0;
      
      /// called when one or more specified contacts is removed
      virtual bool onRemove(resip::ServerRegistrationHandle, const resip::SipMessage& reg)=0;
      
      /// Called when all the contacts are removed using "Contact: *"
      virtual bool onRemoveAll(resip::ServerRegistrationHandle, const resip::SipMessage& reg)=0;
      
      /** Called when one or more contacts are added. This is after 
          authentication has all succeeded */
      virtual bool onAdd(resip::ServerRegistrationHandle, const resip::SipMessage& reg)=0;

      /// Called when a client queries for the list of current registrations
      virtual bool onQuery(resip::ServerRegistrationHandle, const resip::SipMessage& reg)=0;
};

class Registrar: public resip::ServerRegistrationHandler
{
   public:
      Registrar();
      virtual ~Registrar();
      
      void setProxy(Proxy* proxy) { mProxy = proxy; }

      virtual void addRegistrarHandler(RegistrarHandler* handler);

      virtual void onRefresh(resip::ServerRegistrationHandle, const resip::SipMessage& reg);
      virtual void onRemove(resip::ServerRegistrationHandle, const resip::SipMessage& reg);
      virtual void onRemoveAll(resip::ServerRegistrationHandle, const resip::SipMessage& reg);
      virtual void onAdd(resip::ServerRegistrationHandle, const resip::SipMessage& reg);
      virtual void onQuery(resip::ServerRegistrationHandle, const resip::SipMessage& reg);

   private:
      std::list<RegistrarHandler*> mRegistrarHandlers;
      Proxy* mProxy;
};
}
#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
