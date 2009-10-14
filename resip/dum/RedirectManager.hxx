#if !defined(RESIP_REDIRECTMANAGER_HXX)
#define RESIP_REDIRECTMANAGER_HXX


#include <set>
#include <queue>
#include <functional>
#include <vector>

#include "resip/dum/Handles.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/DialogSetId.hxx"
#include "rutil/HashMap.hxx"

//8.1.3.4
//19.1.5

namespace resip
{

class DialogSet;

class RedirectManager
{
   public:   
      class Ordering : public std::binary_function<const NameAddr&, const NameAddr&, bool>
      {
         public:
            virtual ~Ordering() {}
            virtual bool operator()(const NameAddr& lhs, const NameAddr& rhs) const;
      };
      virtual ~RedirectManager() {}
      virtual bool handle(DialogSet& dSet, SipMessage& origRequest, const SipMessage& response);
      
      //deafult is by q-value, no q-value treated as 1.0
      void setOrdering(const Ordering& order);      
      
      void removeDialogSet(DialogSetId id);
      
      //based on follows 8.1.3.4 of 3261. Overload to interject user decisions
      //or to process 301, 305 or 380 reponses.
//      virtual void onRedirect(AppDialogSetHandle, const SipMessage& response);
      //use a priority queue by q-value, and a list(linear, set too heavy?) of
      //values that have been tried. Do q-values really have any meaning across
      //different 3xx values? Or should the first 3xx always win.
   protected:      
      class TargetSet
      {
         public:
            TargetSet(const SipMessage& request, const Ordering& order) :
               mTargetQueue(order),
               mRequest(request)
            {}
            
            void addTargets(const SipMessage& msg);
            //pass in the message stored in the creator
            bool makeNextRequest(SipMessage& request);
         protected:
            typedef std::set<NameAddr> EncounteredTargetSet;      
            typedef std::priority_queue<NameAddr, std::vector<NameAddr>, Ordering> TargetQueue;

            EncounteredTargetSet mTargetSet;
            TargetQueue mTargetQueue;            
            //mLastRequest in creator is kept in sync with this, this is needed
            //so that embedded information in a target uri does not migrate to
            //the wrong attempt
            SipMessage mRequest;                        
      };      

      typedef HashMap<DialogSetId, TargetSet*> RedirectedRequestMap;
      RedirectedRequestMap mRedirectedRequestMap;
      Ordering mOrdering;      
};

 
}

#endif

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
