#ifndef HeaderTypes_hxx
#define HeaderTypes_hxx

#include "sipstack/supported.hxx"
#include "util/Data.hxx"

namespace Vocal2
{

class Headers
{
   public:
      // put headers that you want to appear early in the message early in
      // this set
      enum Type
      {
         To, 
         From, 
         Subject, 
         Call_ID, 
         CSeq, 
         Contact, 
         Via, 
         Route, 
         Expires, 
         Max_Forwards, 
         Accept, 
         Accept_Encoding, 
         Accept_Language, 
         Alert_Info,
         Allow, 
         Authentication_Info, 
         Call_Info, 
         Content_Disposition, 
         Content_Encoding, 
         Content_Language, 
         Content_Type, 
         Date, 
         Error_Info, 
         In_Reply_To, 
         Min_Expires, 
         MIME_Version, 
         Organization, 
         Priority, 
         Proxy_Authenticate, 
         Proxy_Authorization, 
         Proxy_Require, 
         Record_Route, 
         Reply_To, 
         Require, 
         Retry_After, 
         Server, 
         Supported, 
         Timestamp, 
         Unsupported, 
         User_Agent, 
         Warning, 
         WWW_Authenticate,
         Subscription_State,
         Refer_To,
         Referred_By,
         Authorization, 
         Replaces,
         Content_Length, 
         UNKNOWN,
         MAX_HEADERS = UNKNOWN,
         NONE
      };

      static bool CommaTokenizing[MAX_HEADERS];
      static Data HeaderNames[MAX_HEADERS];

      // get enum from header name
      static Type getType(const char* name, int len);
      static bool isCommaTokenizing(Type type);
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
