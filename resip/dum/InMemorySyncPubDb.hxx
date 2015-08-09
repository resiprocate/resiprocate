#if !defined(RESIP_INMEMORYSYNCPUBDB_HXX)
#define RESIP_INMEMORYSYNCPUBDB_HXX

#include <list>

#include "resip/dum/PublicationPersistenceManager.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"

namespace resip
{

class SecurityAttributes;

class InMemorySyncPubDbHandler
{
public:
   typedef enum
   {
      SyncServer,
      AllChanges
   } HandlerMode;
   InMemorySyncPubDbHandler(HandlerMode mode = SyncServer) : mMode(mode) {}
   virtual ~InMemorySyncPubDbHandler(){}
   HandlerMode getMode() { return mMode; }
   virtual void onDocumentModified(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes) = 0;
   virtual void onDocumentRemoved(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated) = 0;
   virtual void onInitialSyncDocument(unsigned int connectionId, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes) {}
protected:
   HandlerMode mMode;
};

/**
  Implementation of a persistence manager. This class keeps
  all publications in memory, and is used for remote replication.

  The InMemorySyncPubDbHandler can be used by an external mechanism to 
  transport publication documents to a remote peer for replication.
  See the RegSyncClient and RegSyncServer implementations in the repro
  project.
*/
class InMemorySyncPubDb : public PublicationPersistenceManager
{
public:

   InMemorySyncPubDb(bool syncEnabled = false);
   virtual ~InMemorySyncPubDb();

   virtual void addHandler(InMemorySyncPubDbHandler* handler);
   virtual void removeHandler(InMemorySyncPubDbHandler* handler);
   virtual void initialSync(unsigned int connectionId);

   // PublicationPersistenceManager Methods
   virtual void addUpdateDocument(const PubDocument& document);
   virtual void addUpdateDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, const Contents* contents, const SecurityAttributes* securityAttributes, bool syncPublication = false);
   virtual bool removeDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated, bool syncPublication = false);
   virtual bool getMergedETags(const Data& eventType, const Data& documentKey, ETagMerger& merger, Contents* destination);
   virtual bool documentExists(const Data& eventType, const Data& documentKey, const Data& eTag);
   virtual bool checkExpired(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated);
   virtual void lockDocuments();
   virtual KeyToETagMap& getDocuments();  // Ensure you lock before calling this and unlock when done
   virtual void unlockDocuments();

protected:

   void invokeOnDocumentModified(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes);
   void invokeOnDocumentRemoved(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated);
   void invokeOnInitialSyncDocument(unsigned int connectionId, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes);
   bool shouldEraseDocument(PubDocument& document, UInt64 now);
   bool mSyncEnabled;
   typedef std::list<InMemorySyncPubDbHandler*> HandlerList;
   HandlerList mHandlers;  // use list over set to preserve add order
   Mutex mHandlerMutex;

   KeyToETagMap mPublicationDb;
   Mutex mDatabaseMutex;
};

}

#endif

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
