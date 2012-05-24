#include "IChatUser.hxx"
#include "JabberComponent.hxx"

using namespace gateway;
using namespace std;

namespace gateway 
{

IChatUser::IChatUser(JabberComponent& component, const std::string& jid) : 
   mJID(jid),
   mComponent(component)
{
}

IChatUser::~IChatUser() 
{
   ResourceMap::iterator it = mResources.begin();
   for(;it!=mResources.end();it++)
   {
      delete it->second;
   }
}

void 
IChatUser::updateResourceInfo(const std::string& resourceId, const gloox::Presence& presence, int priority, bool avAvail)
{
   bool originalUnavailability = isUnavailable();
   if(resourceId.empty())
   {
      if(presence == gloox::PresenceUnavailable)
      {
	     // All resources are unanavaiable
         ResourceMap::iterator it = mResources.begin();
         for(; it != mResources.end(); it++)
         {
            delete it->second;
		 }
		 mResources.clear();
	  }
	  else
	  {
		  // PresenceAvailable with no resource is meaningless
	  }
   }
   else
   {
      ResourceMap::iterator it = mResources.find(resourceId);
      if(it != mResources.end())
      {
         if(presence == gloox::PresenceUnavailable)
         {
            delete it->second;
            mResources.erase(it);
         }
         else
         {
            it->second->mPresence = presence;
            it->second->mPriority = priority;
            it->second->mAvAvail = avAvail;
         }
      }
      else
      {
         if(presence != gloox::PresenceUnavailable)
         {
            mResources[resourceId] = new ResourceInfo(presence, priority, avAvail);
		 }
      }
   }
   if(originalUnavailability && !isUnavailable())
   {
      // User was originally unavailable and now they are available - register SIP endpoint
      mComponent.sipRegisterJabberUser(mJID);
   }
   else if(!originalUnavailability && isUnavailable())
   {
      // User was originally available and now they are unavailable - unregister SIP endpoint
      mComponent.sipUnregisterJabberUser(mJID);      
   }
}

bool 
IChatUser::isUnavailable() 
{ 
   return mResources.empty(); 
}

IChatUser::ResourceMap::iterator 
IChatUser::getMostAvailableResourceItr()
{
   ResourceMap::iterator itBestResource = mResources.end();
   ResourceMap::iterator it = mResources.begin();
   for(;it!=mResources.end();it++)
   {
      if(it->second->mAvAvail)
      {
         if(itBestResource == mResources.end())
         {
            itBestResource=it;
         }
         else
         {
            if(it->second->mPresence < itBestResource->second->mPresence)
            {
               itBestResource=it;
            }
            else if(it->second->mPresence == itBestResource->second->mPresence)
            {
               if(it->second->mPriority < itBestResource->second->mPriority)
               {
                  itBestResource=it;
               }
            }
         }
      }
   }
   return itBestResource;
}

const std::string& 
IChatUser::getMostAvailableResource()
{
   static const std::string empty;
   ResourceMap::iterator itBestResource = getMostAvailableResourceItr();
   if(itBestResource != mResources.end())
   {
      return itBestResource->first;
   }
   return empty;
}

bool
IChatUser::getMostAvailableResourceList(std::list<std::string>& resourceList)
{
   bool ret = false;

   // Pass one - go through list and see what highest mPresence availability is
   ResourceMap::iterator itBestResource = getMostAvailableResourceItr();

   // Pass two - return all resources with matching mPresence and mPriority
   if(itBestResource != mResources.end())
   {
      ResourceMap::iterator it = mResources.begin();
      for(;it!=mResources.end();it++)
      {
         if(it->second->mAvAvail && 
            it->second->mPresence == itBestResource->second->mPresence &&
            it->second->mPriority == itBestResource->second->mPriority)
         {
            resourceList.push_back(it->first);
            ret = true;
         }
      }
   }
   return ret;
}

void 
IChatUser::addSubscribedGatewayUser(const std::string& jid)
{
   mSubscribedGatewayUsers.insert(jid);
}

void 
IChatUser::removeSubscribedGatewayUser(const std::string& jid)
{
   SubscribedGatewayUserList::iterator it = mSubscribedGatewayUsers.find(jid);
   if(it!=mSubscribedGatewayUsers.end())
   {
      mSubscribedGatewayUsers.erase(it);
   }
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

