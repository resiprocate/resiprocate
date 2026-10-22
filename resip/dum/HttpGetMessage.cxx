#include "resip/dum/HttpGetMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;


HttpGetMessage::HttpGetMessage(const Data& tid, 
                               bool success, 
                               const Data& body,
                               const Mime& type) :
   DumFeatureMessage(tid),
   mSuccess(success),
   mBody(body),
   mType(type),
   mStatusCode(200)
{
}

HttpGetMessage::HttpGetMessage(const Data& tid,
                               const Data& userData, 
                               unsigned int statusCode,
                               HeaderMap headers,
                               const Data& body,
                               const Mime& type) :
   DumFeatureMessage(tid),
   mSuccess(statusCode >= 200 && statusCode <= 299),
   mBody(body),
   mType(type),
   mUserData(userData),
   mStatusCode(statusCode),
   mHeaders(std::move(headers))
{
}

const Data&
HttpGetMessage::getHeader(const Data& name) const
{
   // Keys in mHeaders are stored lower-cased - lowercase the lookup key as well
   Data lower(name);
   lower.lowercase();
   HeaderMap::const_iterator it = mHeaders.find(lower);
   return (it != mHeaders.end()) ? it->second : Data::Empty;
}

EncodeStream&
HttpGetMessage::encodeBrief(EncodeStream& strm) const
{ 
   // If UserData is empty and StatusCode is 0, assume 1st constructor was used
   if (mUserData.empty() && mStatusCode == 0)
   {
      return strm << "HttpGetMessage: tid=" << getTransactionId() << ", success=" << Data(mSuccess) << ", bodyType=" << mType;
   }
   else
   {
      return strm << "HttpGetMessage: tid=" << getTransactionId() << ", userData=" << mUserData << ", statusCode=" << mStatusCode << ", bodyType=" << mType;
   }
}

EncodeStream& 
HttpGetMessage::encode(EncodeStream& strm) const
{
   strm << brief() << ", headers={";
   
   bool first = true;
   for (HeaderMap::const_iterator it = mHeaders.begin(); it != mHeaders.end(); ++it)
   {
      if (!first) strm << ", ";
      strm << it->first << ": " << it->second;
      first = false;
   }
   strm << "}, body=" << mBody;
   return strm;
}

Message* 
HttpGetMessage::clone() const 
{ 
   return new HttpGetMessage(getTransactionId(), mUserData, mStatusCode, mHeaders, mBody, mType); 
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026, SIP Spectrum, Inc. https://www.sipspectrum.com
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
