#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

using namespace resip;
using namespace std;

bool
MessageFilterRule::matches()
{
   const Data scheme = msg.header(h_StartLine).uri().scheme()

   if (!schemeIsInList(scheme))
   {
      return false;
   }
   
   if (msg.header(scheme == Symbols::Tel))
   {
      return true;
   }
   else
   {
      if (!hostIsInList( msg.header(h_StartLine).uri().hostpart()))
         return false;

      if (scheme == Symbols::Sip || scheme == Symbols::Sips)
      {   
         int method = msg.header(h_StartLine).method();
         if (!methodIsInList(method))
         {
            return false;
         }
         else
         {
            switch(method)
            {
               case SUBSCRIBE:
               case NOTIFY:
               case PUBLISH:      
                  if (!eventIsInList(msg.header(h_Event).value()))
                  {
                     return false;
                  }
                  break;
               default:
                  break;
            }
         }
      }
   }
}


//defaults for the constructor of a MessageFilterRule
const SchemeList schemes(SIP, SIPS, TEL, IM, PRES, H323);
const MethodList methods(INVITE, ACK, CANCEL, BYE, REGISTER, 
 PUBLISH, SUBSCRIBE, NOTIFY, INFO, OPTIONS, REFER, UPDATE, PRACK, MESSAGE);
// legal values for hostpart comparison are ANY, HOSTISME, DOMAINISME, or a list of Datas
// legal values for events are ANY or a list of Datas


bool
schemeIsInList(Data& scheme)
{
   // step through mSchemeList looking for supported schemes
}

bool
hostpartIsInList(Data& hostpart)
{
   switch(mHostpartMatches)
   {
      case Any:
         return true;
      case HostIsMe:
         return (Helper::hostIsMe(hostpart));  // this func does not exist yet
         break;
      case DomainIsMe:
         return (Helper::domainIsMe(hostpart));  // nor this one
         break;
      case List:
         // walk the list for specific matches
         for (HostpartList::iterator i = mHostpartList.begin() ; mHostpartList.end() ; ++i)
         {
            if (*i.value() == hostpart)
               return true;
         }
         break;
      default:
         break;
   }
   return false;
}


};


//defaults for the constructor of a MessageFilterRule
const SchemeList schemes(SIP, SIPS, TEL, IM, PRES, H323);
const MethodList methods(INVITE, ACK, CANCEL, BYE, REGISTER, 
 PUBLISH, SUBSCRIBE, NOTIFY, INFO, OPTIONS, REFER, UPDATE, PRACK, MESSAGE);
// legal values for hostpart comparison are ANY, HOSTISME, DOMAINISME, or a list of Datas
// legal values for events are ANY or a list of Datas


bool
schemeIsInList(Data& scheme)
{
   // step through mSchemeList looking for supported schemes
}

bool
hostpartIsInList(Data& hostpart)
{
   switch(mHostpartMatches)
   {
      case ANY:
         return true;
      case HOSTISME:
         return (Helper::hostIsMe(hostpart));  // this func does not exist yet
         break;
      case DOMAINISME:
         return (Helper::domainIsMe(hostpart));  // nor this one
         break;
      case LIST:
         // walk the list for specific matches
         for (HostpartList::iterator i = mHostpartList.begin() ; mHostpartList.end() ; ++i)
         {
            if (*i.value() == hostpart)
               return true;
         }
         break;
      default:
         break;
   }
   return false;
}


};


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
