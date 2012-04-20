#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Lock.hxx"

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ExtensionHeader.hxx"

#include "repro/SiloStore.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


SiloStore::SiloStore(AbstractDb& db):
   mDb(db)
{
}


SiloStore::~SiloStore()
{
}

bool
SiloStore::addMessage(const resip::Data& destUri,
                      const resip::Data& sourceUri,
                      time_t originalSendTime,
                      const resip::Data& mimeType,
                      const resip::Data& messageBody)
{
   AbstractDb::SiloRecord rec;
   rec.mDestUri = destUri;
   rec.mSourceUri = sourceUri;
   rec.mOriginalSentTime = originalSendTime;
   rec.mMimeType = mimeType;
   rec.mMessageBody = messageBody;

   WriteLock lock(mMutex);
   return mDb.addToSilo(rec.mDestUri, rec);
}

bool 
SiloStore::getSiloRecords(const AbstractDb::Key& key, AbstractDb::SiloRecordList& recordList)
{
   ReadLock lock(mMutex);
   return mDb.getSiloRecords(key, recordList);
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
 */
