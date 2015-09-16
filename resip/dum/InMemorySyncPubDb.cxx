#include "resip/dum/InMemorySyncPubDb.hxx"
#include "rutil/compat.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

InMemorySyncPubDb::InMemorySyncPubDb(bool syncEnabled) : mSyncEnabled(syncEnabled)
{
}

InMemorySyncPubDb::~InMemorySyncPubDb()
{
}

void 
InMemorySyncPubDb::addHandler(InMemorySyncPubDbHandler* handler)
{ 
   Lock lock(mHandlerMutex);
   mHandlers.push_back(handler); 
}

void 
InMemorySyncPubDb::removeHandler(InMemorySyncPubDbHandler* handler)
{ 
   Lock lock(mHandlerMutex);
   for(HandlerList::iterator it = mHandlers.begin(); it != mHandlers.end(); it++)
   {
       if(*it == handler)
       {
           mHandlers.erase(it);
           break;
       }
   }
}

bool 
InMemorySyncPubDb::shouldEraseDocument(PubDocument& document, UInt64 now)
{
   if (mSyncEnabled)
   {
      // Check if already lingering
      if (document.mExpirationTime == 0)
      {
         // Check if linger time is done
         if (document.mLingerTime <= now)
         {
            return true;
         }
      }
      else if (document.mExpirationTime <= now)
      {
         // Tag document to linger
         document.mLastUpdated = document.mExpirationTime;
         document.mExpirationTime = 0;
      }
   }
   else
   {
      if (document.mExpirationTime <= now)
      {
         return true;
      }
   }
   return false;
}

void
InMemorySyncPubDb::initialSync(unsigned int connectionId)
{
   Lock g(mDatabaseMutex);
   UInt64 now = Timer::getTimeSecs();

   // Iterate through keys
   KeyToETagMap::iterator keyIt = mPublicationDb.begin();
   for (; keyIt != mPublicationDb.end(); )
   {
      // Iterator through documents in sub-map
      ETagToDocumentMap::iterator eTagIt = keyIt->second.begin();
      for (; eTagIt != keyIt->second.end();)
      {
         if (shouldEraseDocument(eTagIt->second, now))
         {
            keyIt->second.erase(eTagIt++);
         }
         else
         {
            invokeOnInitialSyncDocument(connectionId, eTagIt->second.mEventType, eTagIt->second.mDocumentKey, eTagIt->second.mETag, eTagIt->second.mExpirationTime, eTagIt->second.mLastUpdated, eTagIt->second.mContents.get(), eTagIt->second.mSecurityAttributes.get());
            eTagIt++;
         }
      }

      // If there are no more eTags then remove entity
      if (keyIt->second.size() == 0)
      {
         mPublicationDb.erase(keyIt++);
      }
      else
      {
         keyIt++;
      }
   }
}

void 
InMemorySyncPubDb::addUpdateDocument(const PubDocument& document)
{
   Lock g(mDatabaseMutex);
   Data mapKey = document.mEventType + document.mDocumentKey;
   bool found = false;
   KeyToETagMap::iterator keyIt = mPublicationDb.find(mapKey);
   if (keyIt != mPublicationDb.end())
   {
      // Next find eTag in sub-map
      ETagToDocumentMap::iterator eTagIt = keyIt->second.find(document.mETag);
      if (eTagIt != keyIt->second.end())
      {
         // Doc was found!  Do some checks
         found = true;
         // If doc is from sync then ensure it is newer
         if (!document.mSyncPublication || (document.mLastUpdated > eTagIt->second.mLastUpdated))
         {
            UInt64 now = Timer::getTimeSecs();
            SharedPtr<Contents> contentsForOnDocumentModified = document.mContents;
            SharedPtr<SecurityAttributes> securityAttributesForOnDocumentModified = document.mSecurityAttributes;
            // We should only need to linger a document past the latest expiration time we have ever seen, since both sides will
            // treat the publication as gone after this time anyway.  However this is timing sensitive with the sync process.  
            // So we will linger a document for twice this duration.
            UInt64 lingerDuration = (resipMax(document.mExpirationTime, eTagIt->second.mExpirationTime) - now) * 2;
            if (document.mContents.get() == 0)  // If this is a pub refresh then ensure we don't get rid of existing doc body
            {
               // If previous document was expired then ensure we push out a notify on the refresh to tell everyone it's back
               // This can happen if someone deletes a publication on the web page, then it is refreshed.  The delete causes a 
               // notify of closed state, the refresh should bring the state back.
               if (eTagIt->second.mExpirationTime == 0 ||
                   eTagIt->second.mExpirationTime > now)
               {
                   contentsForOnDocumentModified = eTagIt->second.mContents; 
                   securityAttributesForOnDocumentModified = eTagIt->second.mSecurityAttributes;
               }
               SharedPtr<Contents> contents = eTagIt->second.mContents;
               SharedPtr<SecurityAttributes> securityAttributes = eTagIt->second.mSecurityAttributes;
               eTagIt->second = document;
               eTagIt->second.mContents = contents;
               eTagIt->second.mSecurityAttributes = securityAttributes;
            }
            else
            {
               eTagIt->second = document;
            }
            eTagIt->second.mLingerTime = now + lingerDuration;
            // Only pass sync as true if this update just came from an inbound sync operation
            invokeOnDocumentModified(document.mSyncPublication /* sync publication? */, document.mEventType, document.mDocumentKey, document.mETag, document.mExpirationTime, document.mLastUpdated, contentsForOnDocumentModified.get(), securityAttributesForOnDocumentModified.get());
         }
      }
   }

   // If we didn't find an existing document and we have a contents, then add this doc.
   // Note: Pub refreshes don't contain a contents - so we happen to receive a refresh as our
   //       first message for an etag we don't want to add it to the store - until we have 
   //       at least a doc body.
   if (!found && document.mContents.get() != 0)
   {
      // Add new
      mPublicationDb[mapKey][document.mETag] = document;
      // Only pass sync as true if this update just came from an inbound sync operation
      invokeOnDocumentModified(document.mSyncPublication /* sync publication? */, document.mEventType, document.mDocumentKey, document.mETag, document.mExpirationTime, document.mLastUpdated, document.mContents.get(), document.mSecurityAttributes.get());
   }
}

void 
InMemorySyncPubDb::addUpdateDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, const Contents* contents, const SecurityAttributes* securityAttributes, bool syncPublication)
{
   addUpdateDocument(PubDocument(eventType, documentKey, eTag, expirationTime, contents, securityAttributes, syncPublication));
}

bool 
InMemorySyncPubDb::removeDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated, bool syncPublication)
{
   bool result = false;
   Lock g(mDatabaseMutex);

   // First find entity in map
   KeyToETagMap::iterator keyIt = mPublicationDb.find(eventType + documentKey);
   if (keyIt != mPublicationDb.end())
   {
      // Next find eTag in sub-map
      ETagToDocumentMap::iterator eTagIt = keyIt->second.find(eTag);
      if (eTagIt != keyIt->second.end())
      {
         result = true;
         // If remove is from sync then ensure it is newer
         if (!syncPublication || (lastUpdated > eTagIt->second.mLastUpdated))
         {
            // If sync is enabled - then linger the record in memory until it expires
            if (mSyncEnabled)
            {
               // Tag document as expired, but in a linger state
               eTagIt->second.mExpirationTime = 0;
               eTagIt->second.mLastUpdated = Timer::getTimeSecs();
            }
            else
            {
               // ETag was found - remove it
               keyIt->second.erase(eTagIt);
            }
            // Only pass sync as true if this update just come from an inbound sync operation
            invokeOnDocumentRemoved(syncPublication /* sync? */, eventType, documentKey, eTag, lastUpdated);
         }
      }
      // If there are no more eTags then remove entity
      if (keyIt->second.size() == 0)
      {
         mPublicationDb.erase(keyIt);
      }
   }
   return result;
}

bool 
InMemorySyncPubDb::getMergedETags(const Data& eventType, const Data& documentKey, ETagMerger& merger, Contents* destination)
{
   Lock g(mDatabaseMutex);

   // Find entity
   KeyToETagMap::iterator keyIt = mPublicationDb.find(eventType + documentKey);
   if (keyIt != mPublicationDb.end())
   {
      bool isFirst = true;
      UInt64 now = Timer::getTimeSecs();

      // Iterate through all Etags
      ETagToDocumentMap::iterator eTagIt = keyIt->second.begin();
      for (; eTagIt != keyIt->second.end(); )
      {
         if (!shouldEraseDocument(eTagIt->second, now))
         {
            // Just because we don't need to erase it doesn't mean it didn't expire - check for expiration
            if (eTagIt->second.mExpirationTime > now && eTagIt->second.mContents.get() != 0)
            {
               merger.mergeETag(destination, eTagIt->second.mContents.get(), isFirst);
               isFirst = false;
            }
            eTagIt++;
         }
         else
         {
            // ETag has expired - remove it
            keyIt->second.erase(eTagIt++);
            // If no more Etags for key, then remove key entry and bail out
            if (keyIt->second.size() == 0)
            {
               mPublicationDb.erase(keyIt);
               break;
            }
         }
      }
      // If we have at least on ETag then return true
      if (!isFirst)
      {
         return true;
      }
   }
   return false;
}

bool 
InMemorySyncPubDb::documentExists(const Data& eventType, const Data& documentKey, const Data& eTag)
{
   Lock g(mDatabaseMutex);

   // First find entity in map
   KeyToETagMap::iterator keyIt = mPublicationDb.find(eventType + documentKey);
   if (keyIt != mPublicationDb.end())
   {
      // Next find eTag in sub-map
      ETagToDocumentMap::iterator eTagIt = keyIt->second.find(eTag);
      if (eTagIt != keyIt->second.end())
      {
         // Decided not to check if expired or not.  Not checking allows us to handle
         // a scenario where the publication refresh went to another repro node and
         // syncing was broken for some reason.  Then a new publish comes here and
         // the record is still lingering.
         //if (eTagIt->second.mExpirationTime <= Timer::getTimeSecs())
         {
            return true;
         }
      }
   }
   return false;
}

// If lastUpdated != 0 then we make sure that passed in lastUpdated matches the document before returning true
// This method is used in timer expirey and the lastUpdated checks helps us to make sure the timer that just
// expired hasn't been made obsolete due to a new update.
bool InMemorySyncPubDb::checkExpired(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated)
{
   Lock g(mDatabaseMutex);

   // First find entity in map
   KeyToETagMap::iterator keyIt = mPublicationDb.find(eventType + documentKey);
   if (keyIt != mPublicationDb.end())
   {
      // Next find eTag in sub-map
      ETagToDocumentMap::iterator eTagIt = keyIt->second.find(eTag);
      if (eTagIt != keyIt->second.end())
      {
         UInt64 now = Timer::getTimeSecs();
         if (eTagIt->second.mExpirationTime >= now &&
            (lastUpdated == 0 || lastUpdated == eTagIt->second.mLastUpdated))
         {
            DebugLog(<< "InMemorySyncPubDb::checkExpired:  found expired publication, docKey=" << documentKey << ", tag=" << eTag);
            bool syncPublication = eTagIt->second.mSyncPublication;
            // If sync is enabled - then linger the record in memory until it expires
            if (mSyncEnabled)
            {
               // Tag document as expired, but in a linger state
               eTagIt->second.mExpirationTime = 0;
               eTagIt->second.mLastUpdated = now;
            }
            else
            {
               // ETag was found - remove it
               keyIt->second.erase(eTagIt);
               // If no more Etags for key, then remove key entry
               if (keyIt->second.size() == 0)
               {
                  mPublicationDb.erase(keyIt);
               }
            }
            invokeOnDocumentRemoved(syncPublication /* sync? */, eventType, documentKey, eTag, now);
            return true;
         }
      }
   }
   return false;
}

void 
InMemorySyncPubDb::lockDocuments()
{
   mDatabaseMutex.lock();
}

PublicationPersistenceManager::KeyToETagMap& 
InMemorySyncPubDb::getDocuments()
{
   return mPublicationDb;
}

void 
InMemorySyncPubDb::unlockDocuments()
{
   mDatabaseMutex.unlock();
}

void 
InMemorySyncPubDb::invokeOnDocumentModified(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes)
{
   Lock lock(mHandlerMutex);
   for (HandlerList::iterator it = mHandlers.begin(); it != mHandlers.end(); it++)
   {
      // If handler mode is all, then send notification, otherwise handler mode is sync and we ensure passed
      // in syncPublication flag is false - so we don't sync back to originator
      if (!sync || (*it)->getMode() == InMemorySyncPubDbHandler::AllChanges)
      {
         (*it)->onDocumentModified(sync, eventType, documentKey, eTag, expirationTime, lastUpdated, contents, securityAttributes);
      }
   }
}

void 
InMemorySyncPubDb::invokeOnDocumentRemoved(bool sync, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated)
{
   Lock lock(mHandlerMutex);
   for (HandlerList::iterator it = mHandlers.begin(); it != mHandlers.end(); it++)
   {
      // If handler mode is all, then send notification, otherwise handler mode is sync and we ensure passed
      // in syncPublication flag is false - so we don't sync back to originator
      if (!sync || (*it)->getMode() == InMemorySyncPubDbHandler::AllChanges)
      {
         (*it)->onDocumentRemoved(sync, eventType, documentKey, eTag, lastUpdated);
      }
   }
}

void 
InMemorySyncPubDb::invokeOnInitialSyncDocument(unsigned int connectionId, const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, UInt64 lastUpdated, const Contents* contents, const SecurityAttributes* securityAttributes)
{
   Lock lock(mHandlerMutex);
   for (HandlerList::iterator it = mHandlers.begin(); it != mHandlers.end(); it++)
   {
      // If handler mode is all, then send notification, otherwise handler mode is sync and we check the passed
      // in sync flag
      if ((*it)->getMode() == InMemorySyncPubDbHandler::SyncServer)
      {
         (*it)->onInitialSyncDocument(connectionId, eventType, documentKey, eTag, expirationTime, lastUpdated, contents, securityAttributes);
      }
   }
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
