#ifndef SipMessage_hxx
#define SipMessage_hxx

#include "sip2/util/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <list>
#include <vector>
#include <utility>

#include "sip2/sipstack/Headers.hxx"
#include "sip2/sipstack/Message.hxx"
#include "sip2/sipstack/ParserCategories.hxx"
#include "sip2/sipstack/ParserContainer.hxx"
#include "sip2/sipstack/Transport.hxx"
#include "sip2/util/VException.hxx"

namespace Vocal2
{

class SipMessage : public Message
{
   public:
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;

      explicit SipMessage(bool fromWire=false);
      
      SipMessage(const SipMessage& message);

      virtual const Data& getTransactionId() const;
         
      virtual ~SipMessage();

      class Exception : public VException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : VException(msg, file, line) {}

            const char* name() const { return "SipMessage::Exception"; }
      };

      void setFromTU() 
      {
         mIsExternal = false;
      }
      
      bool isExternal() const
      {
         return mIsExternal;
      }

      bool isRequest() const;
      bool isResponse() const;

      virtual std::ostream& encode(std::ostream& str) const;
      
      Data brief() const;

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

      CSeq_Header::Type& header(const CSeq_Header& headerType) const;
      Call_ID_Header::Type& header(const Call_ID_Header& headerType) const;
      Authentication_Info_Header::Type& header(const Authentication_Info_Header& headerType) const;
      Authorization_Header::Type& header(const Authorization_Header& headerType) const;
      Content_Disposition_Header::Type& header(const Content_Disposition_Header& headerType) const;
      Content_Encoding_Header::Type& header(const Content_Encoding_Header& headerType) const;
      Content_Length_Header::Type& header(const Content_Length_Header& headerType) const;
      Content_Type_Header::Type& header(const Content_Type_Header& headerType) const;
      Date_Header::Type& header(const Date_Header& headerType) const;
      Expires_Header::Type& header(const Expires_Header& headerType) const;
      From_Header::Type& header(const From_Header& headerType) const;
      In_Reply_To_Header::Type& header(const In_Reply_To_Header& headerType) const;
      MIME_Version_Header::Type& header(const MIME_Version_Header& headerType) const;
      Max_Forwards_Header::Type& header(const Max_Forwards_Header& headerType) const;
      Min_Expires_Header::Type& header(const Min_Expires_Header& headerType) const;
      Organization_Header::Type& header(const Organization_Header& headerType) const;
      Priority_Header::Type& header(const Priority_Header& headerType) const;
      Proxy_Authenticate_Header::Type& header(const Proxy_Authenticate_Header& headerType) const;
      Proxy_Authorization_Header::Type& header(const Proxy_Authorization_Header& headerType) const;
      Refer_To_Header::Type& header(const Refer_To_Header& headerType) const;
      Referred_By_Header::Type& header(const Referred_By_Header& headerType) const;
      Replaces_Header::Type& header(const Replaces_Header& headerType) const;
      Reply_To_Header::Type& header(const Reply_To_Header& headerType) const;
      Retry_After_Header::Type& header(const Retry_After_Header& headerType) const;
      Server_Header::Type& header(const Server_Header& headerType) const;
      Subject_Header::Type& header(const Subject_Header& headerType) const;
      Timestamp_Header::Type& header(const Timestamp_Header& headerType) const;
      To_Header::Type& header(const To_Header& headerType) const;
      User_Agent_Header::Type& header(const User_Agent_Header& headerType) const;
      WWW_Authenticate_Header::Type& header(const WWW_Authenticate_Header& headerType) const;
      Warning_Header::Type& header(const Warning_Header& headerType) const;

      ParserContainer<Accept_MultiHeader::Type>& header(const Accept_MultiHeader& headerType) const;
      ParserContainer<Accept_Encoding_MultiHeader::Type>& header(const Accept_Encoding_MultiHeader& headerType) const;
      ParserContainer<Accept_Language_MultiHeader::Type>& header(const Accept_Language_MultiHeader& headerType) const;
      ParserContainer<Alert_Info_MultiHeader::Type>& header(const Alert_Info_MultiHeader& headerType) const;
      ParserContainer<Allow_MultiHeader::Type>& header(const Allow_MultiHeader& headerType) const;
      ParserContainer<Call_Info_MultiHeader::Type>& header(const Call_Info_MultiHeader& headerType) const;
      ParserContainer<Contact_MultiHeader::Type>& header(const Contact_MultiHeader& headerType) const;
      ParserContainer<Content_Language_MultiHeader::Type>& header(const Content_Language_MultiHeader& headerType) const;
      ParserContainer<Error_Info_MultiHeader::Type>& header(const Error_Info_MultiHeader& headerType) const;
      ParserContainer<Proxy_Require_MultiHeader::Type>& header(const Proxy_Require_MultiHeader& headerType) const;
      ParserContainer<Record_Route_MultiHeader::Type>& header(const Record_Route_MultiHeader& headerType) const;
      ParserContainer<Require_MultiHeader::Type>& header(const Require_MultiHeader& headerType) const;
      ParserContainer<Route_MultiHeader::Type>& header(const Route_MultiHeader& headerType) const;
      ParserContainer<Subscription_State_MultiHeader::Type>& header(const Subscription_State_MultiHeader& headerType) const;
      ParserContainer<Supported_MultiHeader::Type>& header(const Supported_MultiHeader& headerType) const;
      ParserContainer<Unsupported_MultiHeader::Type>& header(const Unsupported_MultiHeader& headerType) const;
      ParserContainer<Via_MultiHeader::Type>& header(const Via_MultiHeader& headerType) const;

      RequestLine& 
      header(const RequestLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l) const;
      
      // unknown header interface
      StringCategories& header(const Data& symbol) const;
      bool exists(const Data& symbol) const;
      void remove(const Data& symbol);

      // typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const UnknownHeaders& getRawUnknownHeaders() const {return mUnknownHeaders;}

      // transport interface
      void setStartLine(const char* start, int len); 
      void setBody(const char* start, int len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      void setSource(const Transport::Tuple& tuple);
      void setDestination(const Transport::Tuple& dest);
      
      void addBuffer(char* buf);

      bool hasFixedDest() const;
      Data getFixedDest() const;
      void setFixedDest(const Data& dest);
      void clearFixedDest();

      // returns the encoded buffer which was encoded by resolve()
      // should only be called by the TransportSelector
      Data& getEncoded();
      
   private:
      void copyFrom(const SipMessage& message);
      HeaderFieldValueList* ensureHeader(Headers::Type type) const;
      HeaderFieldValueList* ensureHeaders(Headers::Type type) const;

      // not available
      SipMessage& operator=(const SipMessage&);

      bool mIsExternal;
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      mutable UnknownHeaders mUnknownHeaders;
  
      bool mHaveFixedDest;
      Data mFixedDest;

      Transport::Tuple mSource;
      Transport::Tuple mDestination;

      // !dlb! hack out with pre-data pointer in buffer for intrusive list?
      std::vector<char*> mBufferList;
      mutable HeaderFieldValueList* mStartLine;
      mutable HeaderFieldValueList* mBody;
      mutable Data mRFC2543TransactionId;
      mutable bool mRequest;
      mutable bool mResponse;

      Data mEncoded; // to be retransmitted

      friend class TransportSelector;
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
