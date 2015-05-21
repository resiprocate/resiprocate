#include "repro/stateAgents/PresencePublicationHandler.hxx"
#include "repro/stateAgents/PresenceServer.hxx"

#include <resip/dum/DialogUsageManager.hxx>
#include <resip/dum/PublicationPersistenceManager.hxx>
#include <resip/dum/ServerPublication.hxx>
#include <rutil/Logger.hxx>

using namespace repro;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

PresencePublicationHandler::PresencePublicationHandler(resip::DialogUsageManager& dum) 
   : mPublicationDb(dum.getPublicationPersistenceManager())
{
}

void 
PresencePublicationHandler::onInitial(ServerPublicationHandle h, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs, 
                                  UInt32 expires)
{
    if (h->getDocumentKey() == h->getPublisher())  // Ensures this is not a third party publication
    {
       InfoLog(<< "PresencePublicationHandler::onInitial: etag=" << etag << ", expires=" << expires << ", msg=" << std::endl << pub);
       h->send(h->accept(200));
    }
    else
    {
       WarningLog(<< "PresencePublicationHandler::onInitial: etag=" << etag << " rejected since thirdparty publication: dockey=" << h->getDocumentKey() << " doesn't match publisher=" << h->getPublisher());
       h->send(h->accept(403));
    }
}

void 
PresencePublicationHandler::onExpired(ServerPublicationHandle h, const Data& etag)
{
    InfoLog(<< "PresencePublicationHandler::onExpired: etag=" << etag);
}

void 
PresencePublicationHandler::onRefresh(ServerPublicationHandle h, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs,
                                  UInt32 expires)
{
    if (h->getDocumentKey() == h->getPublisher())  // Ensures this is not a third party publication
    {
       InfoLog(<< "PresencePublicationHandler::onRefresh: etag=" << etag << ", expires=" << expires << ", msg=" << std::endl << pub);
       h->send(h->accept(200));
    }
    else
    {
       WarningLog(<< "PresencePublicationHandler::onRefresh: etag=" << etag << " rejected since thirdparty publication: dockey=" << h->getDocumentKey() << " doesn't match publisher=" << h->getPublisher());
       h->send(h->accept(403));
    }
}

void 
PresencePublicationHandler::onUpdate(ServerPublicationHandle h, 
                                 const Data& etag, 
                                 const SipMessage& pub, 
                                 const Contents* contents,
                                 const SecurityAttributes* attrs,
                                 UInt32 expires)
{
   if (h->getDocumentKey() == h->getPublisher())  // Ensures this is not a third party publication
   {
      InfoLog(<< "PresencePublicationHandler::onUpdate: etag=" << etag << ", expires=" << expires << ", msg=" << std::endl << pub);
      h->send(h->accept(200));
   }
   else
   {
      WarningLog(<< "PresencePublicationHandler::onUpdate: etag=" << etag << " rejected since thirdparty publication: dockey=" << h->getDocumentKey() << " doesn't match publisher=" << h->getPublisher());
      h->send(h->accept(403));
   }
}

void 
PresencePublicationHandler::onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, UInt32 expires)
{
    InfoLog(<< "PresencePublicationHandler::onRemoved: etag=" << etag << ", expires=" << expires << ", msg=" << std::endl << pub);
}


/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
