#include "rutil/ResipAssert.h"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "resip/dum/HandleManager.hxx"
#include "resip/dum/HandleException.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

HandleManager::HandleManager() : 
   mShuttingDown(false),
   mLastId(Handled::npos)
{
}

HandleManager::~HandleManager()
{
   // !jf! do nothing?
   // !dcm! -- this is the best we can do w/out a back-ptr to each handle
   // DUM currently cleans up properly, so not an issue unless users make their
   // own handled objects, could clean up memeory, but the app will crash first
   // handle deference regardless.
   if (!mHandleMap.empty())
   {
      DebugLog ( << "&&&&&& HandleManager::~HandleManager: Deleting handlemanager that still has Handled objects: " );
      DebugLog ( << InserterP(mHandleMap));   
      //throw HandleException("Deleting handlemanager that still has Handled objects", __FILE__, __LINE__);
   }
}

Handled::Id
HandleManager::create(Handled* handled)
{
   mHandleMap[++mLastId] = handled;
   return mLastId;
}

void HandleManager::shutdownWhenEmpty()
{
   mShuttingDown = true;
   if (mHandleMap.empty())
   {
      onAllHandlesDestroyed();      
   }
   else
   {
      DebugLog (<< "Shutdown waiting for all usages to be deleted (" << mHandleMap.size() << ")");
#if 1
      for (HandleMap::iterator i=mHandleMap.begin() ; i != mHandleMap.end(); ++i)
      {
         DebugLog (<< i->first << " -> " << *(i->second));
      }
#endif
   }
}

#if 0
// !jf! this will leak if there are active usages
void HandleManager::onAllHandlesDestroyed()
{
   WarningLog(<< "Forcing shutdown " << mHandleMap.size() << " active usages");
   for (HandleMap::const_iterator i = mHandleMap.begin();
        i != mHandleMap.end(); ++i)
   {
      InfoLog(<< "Handled left at force shutdown: " << *i->second);
   }
   mHandleMap.clear();
}
#endif

void
HandleManager::remove(Handled::Id id)
{
   HandleMap::iterator i = mHandleMap.find(id);
   resip_assert (i != mHandleMap.end());
   mHandleMap.erase(i);
   if (mShuttingDown)
   {
      if(mHandleMap.empty())
      {
         onAllHandlesDestroyed();      
      }
      else
      {
         DebugLog (<< "Waiting for usages to be deleted (" << mHandleMap.size() << ")");      
      }
   }
}

void
HandleManager::dumpHandles() const
{
   DebugLog (<< "Waiting for usages to be deleted (" << mHandleMap.size() << ")");
   for (HandleMap::const_iterator i=mHandleMap.begin() ; i != mHandleMap.end(); ++i)
   {
      DebugLog (<< i->first << " -> " << *(i->second));
   }
}

bool
HandleManager::isValidHandle(Handled::Id id) const
{
   //!dcm! -- fix; use find
   return mHandleMap.count(id) != 0;
}

Handled*
HandleManager::getHandled(Handled::Id id) const
{
   HandleMap::const_iterator i = mHandleMap.find(id);
   if (i == mHandleMap.end())
   {
      InfoLog (<< "Reference to stale handle: " << id);
      resip_assert(0);
      throw HandleException("Stale handle", __FILE__, __LINE__);
   }
   else
   {
      resip_assert(i->second);
      return i->second;
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

