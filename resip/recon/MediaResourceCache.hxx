#if !defined(MediaResourceCache_hxx)
#define MediaResourceCache_hxx

#include <map>
#include <rutil/Mutex.hxx>

namespace recon
{

/**
  This class is responsible for caching media resource buffers.  It use a Mutex
  for locking, so that additions can happen from other threads.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/

class MediaResourceCache
{
   public:  
      MediaResourceCache();
      virtual ~MediaResourceCache();
      void addToCache(const resip::Data& name, const resip::Data& buffer, int type);
      bool getFromCache(const resip::Data& name, resip::Data** buffer, int* type);

   private:
      class CacheItem
      {
      public:
         CacheItem(const resip::Data& buffer, int type) :
            mBuffer(buffer), mType(type) {}
         resip::Data mBuffer;
         int mType;
      };

      typedef std::map<resip::Data,CacheItem*> CacheMap;
      CacheMap mCacheMap;
      resip::Mutex mMutex;
};

}

#endif


/* ====================================================================

 Copyright (c) 2021, SIP Spectrum, Inc. www.sipspectrum.com
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
