#include "ReconSubsystem.hxx"
#include "MediaResourceCache.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

MediaResourceCache::MediaResourceCache()
{
}

MediaResourceCache::~MediaResourceCache()
{
   // Clear memory
   CacheMap::iterator it;
   for(it = mCacheMap.begin(); it != mCacheMap.end(); it++)
   {
      delete it->second;
   }
}

void 
MediaResourceCache::addToCache(const resip::Data& name, const resip::Data& buffer, int type)
{
   Lock lock(mMutex);

   CacheMap::iterator it = mCacheMap.find(name);
   if(it != mCacheMap.end())
   {
      // Item already present update it
      // SLG - WARNING - if this cached item is currently being played - there WILL be issues - need to fix this, if calling this at runtime is a requirement
      it->second->mBuffer = buffer;  // copies buffer locally, so that caller can free
      it->second->mType = type;
   }
   else
   {
      // Add new item
      mCacheMap[name] = new CacheItem(buffer, type);
   }
}

bool 
MediaResourceCache::getFromCache(const resip::Data& name, resip::Data** buffer, int* type)
{
   Lock lock(mMutex);

   CacheMap::iterator it = mCacheMap.find(name);
   if(it != mCacheMap.end())
   {
      *buffer = &it->second->mBuffer;
      *type = it->second->mType;
      return true;
   }
   return false;
}


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
