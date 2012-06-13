#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "repro/Registrar.hxx"
#include "repro/Proxy.hxx"
#include "resip/dum/ServerRegistration.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

Registrar::Registrar() : mProxy(0)
{
}

Registrar::~Registrar()
{
}

void 
Registrar::addRegistrarHandler(RegistrarHandler* handler)
{
   mRegistrarHandlers.push_back(handler);
}

void 
Registrar::onRefresh(resip::ServerRegistrationHandle sr,
                     const resip::SipMessage& reg)
{
   DebugLog (<< "Registrar::onRefresh " << reg.brief());
   std::list<RegistrarHandler*>::iterator it = mRegistrarHandlers.begin();
   bool continueProcessing = true;
   for(; it != mRegistrarHandlers.end() && continueProcessing; it++)
   {
      continueProcessing = (*it)->onRefresh(sr, reg);
   }
   if(continueProcessing)
   {
      if(mProxy)
      {
         mProxy->doRegistrationAccounting(AccountingCollector::RegistrationRefreshed, reg);
      }
      sr->accept();
   }
}

void 
Registrar::onRemove(resip::ServerRegistrationHandle sr,
                    const resip::SipMessage& reg)
{
   DebugLog (<< "Registrar::onRemove " << reg.brief());
   std::list<RegistrarHandler*>::iterator it = mRegistrarHandlers.begin();
   bool continueProcessing = true;
   for(; it != mRegistrarHandlers.end() && continueProcessing; it++)
   {
      continueProcessing = (*it)->onRemove(sr, reg);
   }
   if(continueProcessing)
   {
      if(mProxy)
      {
         mProxy->doRegistrationAccounting(AccountingCollector::RegistrationRemoved, reg);
      }
      sr->accept();
   }
}
      
void 
Registrar::onRemoveAll(resip::ServerRegistrationHandle sr,
                       const resip::SipMessage& reg)
{
   DebugLog (<< "Registrar::onRemoveAll " << reg.brief());
   std::list<RegistrarHandler*>::iterator it = mRegistrarHandlers.begin();
   bool continueProcessing = true;
   for(; it != mRegistrarHandlers.end() && continueProcessing; it++)
   {
      continueProcessing = (*it)->onRemoveAll(sr, reg);
   }
   if(continueProcessing)
   {
      if(mProxy)
      {
         mProxy->doRegistrationAccounting(AccountingCollector::RegistrationRemovedAll, reg);
      }
      sr->accept();
   }
}
      
void 
Registrar::onAdd(resip::ServerRegistrationHandle sr,
                 const resip::SipMessage& reg)
{
   DebugLog (<< "Registrar::onAdd " << reg.brief());
   std::list<RegistrarHandler*>::iterator it = mRegistrarHandlers.begin();
   bool continueProcessing = true;
   for(; it != mRegistrarHandlers.end() && continueProcessing; it++)
   {
      continueProcessing = (*it)->onAdd(sr, reg);
   }
   if(continueProcessing)
   {
      if(mProxy)
      {
         mProxy->doRegistrationAccounting(AccountingCollector::RegistrationAdded, reg);
      }
      sr->accept();
   }
}
      
void 
Registrar::onQuery(resip::ServerRegistrationHandle sr,
                   const resip::SipMessage& reg)
{
   std::list<RegistrarHandler*>::iterator it = mRegistrarHandlers.begin();
   bool continueProcessing = true;
   for(; it != mRegistrarHandlers.end() && continueProcessing; it++)
   {
      continueProcessing = (*it)->onQuery(sr, reg);
   }
   if(continueProcessing)
   {
      sr->accept();
   }
}

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
