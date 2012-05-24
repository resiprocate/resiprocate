#if !defined(IChatUser_hxx)
#define IChatUser_hxx

#include <map>
#include <set>
#include <list>

#ifdef WIN32
#define RESIP_CONTRIB_GLOOX
#endif

// Gloox includes
#ifndef RESIP_CONTRIB_GLOOX
#include <gloox/component.h>
#include <gloox/presence.h>
#else
#include <src/component.h>
#include <src/presence.h>
#endif

namespace gateway
{

class JabberComponent;

class ResourceInfo
{
public:
   ResourceInfo(const gloox::Presence& presence, int priority, bool avAvail) :
      mPresence(presence), mPriority(priority), mAvAvail(avAvail) {}
   ~ResourceInfo() {}

   gloox::Presence mPresence;
   int mPriority;
   bool mAvAvail;
};

class IChatUser
{
public:
   IChatUser(JabberComponent& component, const std::string& jid);
   ~IChatUser();

   typedef std::set<std::string> SubscribedGatewayUserList;

   void updateResourceInfo(const std::string& resourceId, const gloox::Presence& presence, int priority, bool avAvail);
   bool isUnavailable();
   const std::string& getMostAvailableResource();
   bool getMostAvailableResourceList(std::list<std::string>& resourceList);

   void addSubscribedGatewayUser(const std::string& jid);
   void removeSubscribedGatewayUser(const std::string& jid);
   const SubscribedGatewayUserList& getSubscribedGatewayUserList() { return mSubscribedGatewayUsers; }

private:
   std::string mJID;

   typedef std::map<std::string, ResourceInfo*> ResourceMap;
   ResourceMap mResources;

   ResourceMap::iterator getMostAvailableResourceItr();

   SubscribedGatewayUserList mSubscribedGatewayUsers;
   JabberComponent& mComponent;
};

}

#endif

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

