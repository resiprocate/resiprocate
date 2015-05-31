#if !defined(RegSyncClient_hxx)
#define RegSyncClient_hxx 

#include <rutil/Data.hxx>
#include <rutil/XMLCursor.hxx>
#include <resip/dum/InMemorySyncRegDb.hxx>
#include <resip/dum/InMemorySyncPubDb.hxx>
#include <rutil/ThreadIf.hxx>

namespace repro
{

class RegSyncClient : public resip::ThreadIf
{
public:
   RegSyncClient(resip::InMemorySyncRegDb* regDb,
                 resip::Data address,
                 unsigned short port,
                 resip::InMemorySyncPubDb* pubDb = 0);

   virtual void thread();
   virtual void shutdown();

private: 
   void delaySeconds(unsigned int seconds);
   bool tryParse();  // returns true if we processed something and there is more data in the buffer
   void handleXml(const resip::Data& xmlData);
   void handleRegInfoEvent(resip::XMLCursor& xml);
   void handlePubInfoEvent(resip::XMLCursor& xml);
   void processModify(const resip::Uri& aor, resip::ContactList& syncContacts);

   resip::InMemorySyncRegDb* mRegDb;
   resip::InMemorySyncPubDb* mPubDb;
   resip::Data mAddress;
   unsigned short mPort;
   char mRxBuffer[8000];
   resip::Data mRxDataBuffer;
   int mSocketDesc;
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

