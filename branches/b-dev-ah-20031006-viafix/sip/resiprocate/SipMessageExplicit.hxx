// Included in the body of SipMessage

#if 0
class SipMessage
{
#endif    

#define EXISTS(_T)                                              \
      bool exists(const Header<Headers::_T>& headerType) const

      EXISTS(CSeq);
      EXISTS(Call_ID);
      EXISTS(Contact);
      EXISTS(Content_Length);
      EXISTS(Expires);
      EXISTS(From);
      EXISTS(Max_Forwards);
      EXISTS(Route);
      EXISTS(Subject);
      EXISTS(To);
      EXISTS(Via);
      EXISTS(Accept);
      EXISTS(Accept_Encoding);
      EXISTS(Accept_Language);
      EXISTS(Alert_Info);
      EXISTS(Allow);
      EXISTS(Authentication_Info);
      EXISTS(Call_Info);
      EXISTS(Content_Disposition);
      EXISTS(Content_Encoding);
      EXISTS(Content_Language);
      EXISTS(Content_Type);
      EXISTS(Date);
      EXISTS(Error_Info);
      EXISTS(In_Reply_To);
      EXISTS(Min_Expires);
      EXISTS(MIME_Version);
      EXISTS(Organization);
      EXISTS(Priority);
      EXISTS(Proxy_Authenticate);
      EXISTS(Proxy_Authorization);
      EXISTS(Proxy_Require);
      EXISTS(Record_Route);
      EXISTS(Reply_To);
      EXISTS(Require);
      EXISTS(Retry_After);
      EXISTS(Server);
      EXISTS(Supported);
      EXISTS(Timestamp);
      EXISTS(Unsupported);
      EXISTS(User_Agent);
      EXISTS(Warning);
      EXISTS(WWW_Authenticate);
      EXISTS(Subscription_State);
      EXISTS(Refer_To);
      EXISTS(Referred_By);
      EXISTS(Authorization);
      EXISTS(Replaces);
             
#define HEADER(_T)                                                                      \
      Header<Headers::_T>::Type& header(const Header<Headers::_T>& headerType) const

      HEADER(CSeq);
      HEADER(Call_ID);
      HEADER(Authentication_Info);
      HEADER(Authorization);
      HEADER(Content_Disposition);
      HEADER(Content_Encoding);
      HEADER(Content_Length);
      HEADER(Content_Type);
      HEADER(Date);
      HEADER(Expires);
      HEADER(From);
      HEADER(In_Reply_To);
      HEADER(MIME_Version);
      HEADER(Max_Forwards);
      HEADER(Min_Expires);
      HEADER(Organization);
      HEADER(Priority);
      HEADER(Proxy_Authenticate);
      HEADER(Proxy_Authorization);
      HEADER(Refer_To);
      HEADER(Referred_By);
      HEADER(Replaces);
      HEADER(Reply_To);
      HEADER(Retry_After);
      HEADER(Server);
      HEADER(Subject);
      HEADER(Timestamp);
      HEADER(To);
      HEADER(User_Agent);
      HEADER(WWW_Authenticate);
      HEADER(Warning);

#define MULTI_HEADER(_T) \
      ParserContainer<MultiHeader<Headers::_T>::Type>& header(const MultiHeader<Headers::_T>& headerType) const

      MULTI_HEADER(Accept);
      MULTI_HEADER(Accept_Encoding);
      MULTI_HEADER(Accept_Language);
      MULTI_HEADER(Alert_Info);
      MULTI_HEADER(Allow);
      MULTI_HEADER(Call_Info);
      MULTI_HEADER(Contact);
      MULTI_HEADER(Content_Language);
      MULTI_HEADER(Error_Info);
      MULTI_HEADER(Proxy_Require);
      MULTI_HEADER(Record_Route);
      MULTI_HEADER(Require);
      MULTI_HEADER(Route);
      MULTI_HEADER(Subscription_State);
      MULTI_HEADER(Supported);
      MULTI_HEADER(Unsupported);
      MULTI_HEADER(Via);

#define REMOVE(_T) \
      void remove(const Header<Headers::_T>& headerType)

      REMOVE(CSeq);
      REMOVE(Call_ID);
      REMOVE(Contact);
      REMOVE(Content_Length);
      REMOVE(Expires);
      REMOVE(From);
      REMOVE(Max_Forwards);
      REMOVE(Route);
      REMOVE(Subject);
      REMOVE(To);
      REMOVE(Via);
      REMOVE(Accept);
      REMOVE(Accept_Encoding);
      REMOVE(Accept_Language);
      REMOVE(Alert_Info);
      REMOVE(Allow);
      REMOVE(Authentication_Info);
      REMOVE(Call_Info);
      REMOVE(Content_Disposition);
      REMOVE(Content_Encoding);
      REMOVE(Content_Language);
      REMOVE(Content_Type);
      REMOVE(Date);
      REMOVE(Error_Info);
      REMOVE(In_Reply_To);
      REMOVE(Min_Expires);
      REMOVE(MIME_Version);
      REMOVE(Organization);
      REMOVE(Priority);
      REMOVE(Proxy_Authenticate);
      REMOVE(Proxy_Authorization);
      REMOVE(Proxy_Require);
      REMOVE(Record_Route);
      REMOVE(Reply_To);
      REMOVE(Require);
      REMOVE(Retry_After);
      REMOVE(Server);
      REMOVE(Supported);
      REMOVE(Timestamp);
      REMOVE(Unsupported);
      REMOVE(User_Agent);
      REMOVE(Warning);
      REMOVE(WWW_Authenticate);
      REMOVE(Subscription_State);
      REMOVE(Refer_To);
      REMOVE(Referred_By);
      REMOVE(Authorization);
      REMOVE(Replaces);
#if 0
}
#endif

#undef EXISTS
#undef HEADERS
#undef MULTI_HEADERS
#undef REMOVE


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
