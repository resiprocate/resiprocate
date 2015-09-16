#if !defined(RegSyncServer_hxx)
#define RegSyncServer_hxx 

#include <rutil/Data.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/XMLCursor.hxx>
#include <resip/dum/InMemorySyncRegDb.hxx>
#include <resip/dum/InMemorySyncPubDb.hxx>
#include "repro/XmlRpcServerBase.hxx"

#define REGSYNC_VERSION 4

namespace repro
{
class RegSyncServer;

class RegSyncServer: public XmlRpcServerBase, 
                     public resip::InMemorySyncRegDbHandler,
                     public resip::InMemorySyncPubDbHandler
{
public:
   RegSyncServer(resip::InMemorySyncRegDb* regDb,
                 int port, 
                 resip::IpVersion version,
                 resip::InMemorySyncPubDb* pubDb = 0);
   virtual ~RegSyncServer();

   // thread safe
   virtual void sendResponse(unsigned int connectionId, 
                             unsigned int requestId, 
                             const resip::Data& responseData, 
                             unsigned int resultCode, 
                             const resip::Data& resultText);

   // Use connectionId == 0 to send to all connections
   virtual void sendRegistrationModifiedEvent(unsigned int connectionId, const resip::Uri& aor);
   virtual void sendRegistrationModifiedEvent(unsigned int connectionId, const resip::Uri& aor, const resip::ContactList& contacts);
   virtual void sendDocumentModifiedEvent(unsigned int connectionId, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const resip::Contents* contents, const resip::SecurityAttributes* securityAttributes);
   virtual void sendDocumentRemovedEvent(unsigned int connectionId, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated);

protected:
   virtual void handleRequest(unsigned int connectionId, unsigned int requestId, const resip::Data& request); 

   // InMemorySyncRegDbHandler methods
   virtual void onAorModified(const resip::Uri& aor, const resip::ContactList& contacts);
   virtual void onInitialSyncAor(unsigned int connectionId, const resip::Uri& aor, const resip::ContactList& contacts);

   // InMemorySyncPubDbHandler methods
   virtual void onDocumentModified(bool sync, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const resip::Contents* contents, const resip::SecurityAttributes* securityAttributes);
   virtual void onDocumentRemoved(bool sync, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 lastUpdated);
   virtual void onInitialSyncDocument(unsigned int connectionId, const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const resip::Contents* contents, const resip::SecurityAttributes* securityAttributes);

private: 
   void handleInitialSyncRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void streamContactInstanceRecord(std::stringstream& ss, const resip::ContactInstanceRecord& rec);

   resip::InMemorySyncRegDb* mRegDb;
   resip::InMemorySyncPubDb* mPubDb;
};

}

#endif  

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
