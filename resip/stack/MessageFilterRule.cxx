#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/MessageFilterRule.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TRANSACTION

using namespace resip;
using namespace std;


MessageFilterRule::MessageFilterRule(SchemeList    schemeList,
                                     HostpartTypes hostpartType,
                                     MethodList    methodList,
                                     EventList     eventList)
  : mSchemeList(schemeList),
    mHostpartMatches(hostpartType),
    mMethodList(methodList),
    mEventList(eventList),
	mTransactionUser(0)
{
}

MessageFilterRule::MessageFilterRule(SchemeList    schemeList,
                                     HostpartList  hostpartList,
                                     MethodList    methodList,
                                     EventList     eventList)
  : mSchemeList(schemeList),
    mHostpartMatches(List),
    mHostpartList(hostpartList),
    mMethodList(methodList),
    mEventList(eventList),
	mTransactionUser(0)
{
}

bool
MessageFilterRule::matches(const SipMessage &msg) const
{
   const Data scheme = msg.header(h_RequestLine).uri().scheme();

   if (!schemeIsInList(scheme))
   {
      DebugLog(<< "  MessageFilterRule::matches: Scheme is not in list. Rule does not match.");
      return false;
   }
   
   if (msg.header(h_RequestLine).uri().scheme() != Symbols::Tel)
   {
      // !rwm! Should be hostport, not host
      if (!hostIsInList( msg.header(h_RequestLine).uri().host()))
      {
         DebugLog(<< "  MessageFilterRule::matches: Host is not in list. Rule does not match.");
         return false;
      }
   }

   MethodTypes method = msg.header(h_RequestLine).method();
   if (!methodIsInList(method))
   {
      DebugLog(<< "  MessageFilterRule::matches: Method is not in list. Rule does not match.");
      return false;
   }
   else
   {
      switch(method)
      {
         case SUBSCRIBE:
         case NOTIFY:
         case PUBLISH:      
            if (!eventIsInList(msg))
            {
               DebugLog(<< "  MessageFilterRule::matches: Event is not in list. Rule does not match.");
               return false;
            }
            break;
         default:
         break;
      }
   }

   return true;
}

bool
MessageFilterRule::schemeIsInList(const Data& scheme) const
{
   // Emtpy list means "sip or sips"
   if (mSchemeList.empty())
   {
      return (scheme == Symbols::Sip || scheme == Symbols::Sips || scheme == Symbols::Tel);
   }

   // step through mSchemeList looking for supported schemes
   for (SchemeList::const_iterator i = mSchemeList.begin();
        i != mSchemeList.end(); i++)
   {
      if (scheme == *i)
      {
         return true;
      }
      
   }
   return false;
}

bool
MessageFilterRule::methodIsInList(MethodTypes method) const
{
   // empty list means "match all"
   if (mMethodList.empty())
   {
      return true;
   }

   for (MethodList::const_iterator i = mMethodList.begin();
        i != mMethodList.end(); i++)
   {
      if (method == *i)
      {
         return true;
      }
      
   }
   return false;
}

bool
MessageFilterRule::eventIsInList(const SipMessage& msg) const
{
   // empty list means "match all"
   if (mEventList.empty())
   {
      return true;
   }

   if (!msg.exists(h_Event))
   {
      return false;
   }
   
   Data event = msg.header(h_Event).value();   

   for (EventList::const_iterator i = mEventList.begin();
        i != mEventList.end(); i++)
   {
      if (event == *i)
      {
         return true;
      }
      
   }
   return false;
}

bool
MessageFilterRule::hostIsInList(const Data& hostpart) const
{
   switch(mHostpartMatches)
   {
      case Any:
         return true;
      case HostIsMe:
           // !abr! Waiting for TU support for this method.
           // return (tu.hostIsMe(hostpart));
           return false;
         break;
      case DomainIsMe:
           if(mTransactionUser)
           {
              return mTransactionUser->isMyDomain(hostpart);
           }
           return false;
         break;
      case List:
         // walk the list for specific matches
         for (HostpartList::const_iterator i = mHostpartList.begin() ; 
              i != mHostpartList.end() ; ++i)
         {
            if (isEqualNoCase(*i, hostpart))
               return true;
         }
         break;
      default:
         break;
   }
   return false;
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
