#ifndef SipMessage_hxx
#define SipMessage_hxx

#include "resiprocate/util/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <list>
#include <vector>
#include <utility>

#include "resiprocate/sipstack/Headers.hxx"
#include "resiprocate/sipstack/Message.hxx"
#include "resiprocate/sipstack/ParserCategories.hxx"
#include "resiprocate/sipstack/ParserContainer.hxx"
#include "resiprocate/sipstack/Transport.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/util/BaseException.hxx"
#include "resiprocate/util/Timer.hxx"

namespace Vocal2
{

class Contents;
class UnknownHeaderType;

class SipMessage : public Message
{
   public:
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;

      class FromWireType
      {
            friend class SipMessage;
      };
      static const FromWireType *FromWire;
      static const FromWireType *NotFromWire;

      explicit SipMessage(const FromWireType* fromWire = SipMessage::NotFromWire);
      
      SipMessage(const SipMessage& message);

      // returns the transaction id from the branch or if 2543, the computed hash
      virtual const Data& getTransactionId() const;

      const Data& getRFC2543TransactionId() const;;
      void setRFC2543TransactionId(const Data& tid);
      
      virtual ~SipMessage();

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "SipMessage::Exception"; }
      };

      void setFromTU() 
      {
         mIsExternal = false;
      }

      void setFromExternal()
      {
         mIsExternal = true;
      }
      
      bool isExternal() const
      {
         return mIsExternal;
      }

      virtual std::ostream& encode(std::ostream& str) const;
      std::ostream& encodeEmbedded(std::ostream& str) const;
      
      Data brief() const;

      bool isRequest() const;
      bool isResponse() const;

      RequestLine& 
      header(const RequestLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l) const;

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

#ifdef PARTIAL_TEMPLATE_SPECIALIZATION

      template <class T>
      typename T::UnknownReturn&
      SipMessage::header(const T& headerType) const
      {
         HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), T::Single);
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<typename T::Type>(hfvs, headerType.getTypeNum()));
         }
         return T::knownReturn(hfvs->getParserContainer());
      };

#else

      CSeq_Header::Type& header(const CSeq_Header& headerType) const;
      CallId_Header::Type& header(const CallId_Header& headerType) const;
      AuthenticationInfo_Header::Type& header(const AuthenticationInfo_Header& headerType) const;
      ContentDisposition_Header::Type& header(const ContentDisposition_Header& headerType) const;
      ContentTransferEncoding_Header::Type& header(const ContentTransferEncoding_Header& headerType) const;
      ContentEncoding_Header::Type& header(const ContentEncoding_Header& headerType) const;
      ContentLength_Header::Type& header(const ContentLength_Header& headerType) const;
      ContentType_Header::Type& header(const ContentType_Header& headerType) const;
      Date_Header::Type& header(const Date_Header& headerType) const;
      Event_Header::Type& header(const Event_Header& headerType) const;
      Expires_Header::Type& header(const Expires_Header& headerType) const;
      From_Header::Type& header(const From_Header& headerType) const;
      InReplyTo_Header::Type& header(const InReplyTo_Header& headerType) const;
      MIMEVersion_Header::Type& header(const MIMEVersion_Header& headerType) const;
      MaxForwards_Header::Type& header(const MaxForwards_Header& headerType) const;
      MinExpires_Header::Type& header(const MinExpires_Header& headerType) const;
      Organization_Header::Type& header(const Organization_Header& headerType) const;
      Priority_Header::Type& header(const Priority_Header& headerType) const;
      ReferTo_Header::Type& header(const ReferTo_Header& headerType) const;
      ReferredBy_Header::Type& header(const ReferredBy_Header& headerType) const;
      Replaces_Header::Type& header(const Replaces_Header& headerType) const;
      ReplyTo_Header::Type& header(const ReplyTo_Header& headerType) const;
      RetryAfter_Header::Type& header(const RetryAfter_Header& headerType) const;
      Server_Header::Type& header(const Server_Header& headerType) const;
      Subject_Header::Type& header(const Subject_Header& headerType) const;
      Timestamp_Header::Type& header(const Timestamp_Header& headerType) const;
      To_Header::Type& header(const To_Header& headerType) const;
      UserAgent_Header::Type& header(const UserAgent_Header& headerType) const;
      Warning_Header::Type& header(const Warning_Header& headerType) const;

      ParserContainer<SecurityClient_MultiHeader::Type>& header(const SecurityClient_MultiHeader& headerType) const;
      ParserContainer<SecurityServer_MultiHeader::Type>& header(const SecurityServer_MultiHeader& headerType) const;
      ParserContainer<SecurityVerify_MultiHeader::Type>& header(const SecurityVerify_MultiHeader& headerType) const;

      ParserContainer<Authorization_MultiHeader::Type>& header(const Authorization_MultiHeader& headerType) const;
      ParserContainer<ProxyAuthenticate_MultiHeader::Type>& header(const ProxyAuthenticate_MultiHeader& headerType) const;
      ParserContainer<WWWAuthenticate_MultiHeader::Type>& header(const WWWAuthenticate_MultiHeader& headerType) const;
      ParserContainer<ProxyAuthorization_MultiHeader::Type>& header(const ProxyAuthorization_MultiHeader& headerType) const;

      ParserContainer<Accept_MultiHeader::Type>& header(const Accept_MultiHeader& headerType) const;
      ParserContainer<AcceptEncoding_MultiHeader::Type>& header(const AcceptEncoding_MultiHeader& headerType) const;
      ParserContainer<AcceptLanguage_MultiHeader::Type>& header(const AcceptLanguage_MultiHeader& headerType) const;
      ParserContainer<AlertInfo_MultiHeader::Type>& header(const AlertInfo_MultiHeader& headerType) const;
      ParserContainer<Allow_MultiHeader::Type>& header(const Allow_MultiHeader& headerType) const;
      ParserContainer<CallInfo_MultiHeader::Type>& header(const CallInfo_MultiHeader& headerType) const;
      ParserContainer<Contact_MultiHeader::Type>& header(const Contact_MultiHeader& headerType) const;
      ParserContainer<ContentLanguage_MultiHeader::Type>& header(const ContentLanguage_MultiHeader& headerType) const;
      ParserContainer<ErrorInfo_MultiHeader::Type>& header(const ErrorInfo_MultiHeader& headerType) const;
      ParserContainer<ProxyRequire_MultiHeader::Type>& header(const ProxyRequire_MultiHeader& headerType) const;
      ParserContainer<RecordRoute_MultiHeader::Type>& header(const RecordRoute_MultiHeader& headerType) const;
      ParserContainer<Require_MultiHeader::Type>& header(const Require_MultiHeader& headerType) const;
      ParserContainer<Route_MultiHeader::Type>& header(const Route_MultiHeader& headerType) const;
      ParserContainer<SubscriptionState_MultiHeader::Type>& header(const SubscriptionState_MultiHeader& headerType) const;
      ParserContainer<Supported_MultiHeader::Type>& header(const Supported_MultiHeader& headerType) const;
      ParserContainer<Unsupported_MultiHeader::Type>& header(const Unsupported_MultiHeader& headerType) const;
      ParserContainer<Via_MultiHeader::Type>& header(const Via_MultiHeader& headerType) const;

#endif // METHOD_TEMPLATES

      // unknown header interface
      StringCategories& header(const UnknownHeaderType& symbol) const;
      bool exists(const UnknownHeaderType& symbol) const;
      void remove(const UnknownHeaderType& symbol);

      // typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const UnknownHeaders& getRawUnknownHeaders() const {return mUnknownHeaders;}

      Contents* getContents() const;
      void setContents(const Contents* contents);

      // transport interface
      void setStartLine(const char* start, int len); 

      void setBody(const char* start, int len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      void setSource(const Transport::Tuple& tuple) { mSource = tuple; }
      const Transport::Tuple& getSource() const { return mSource; }

      void setDestination(const Transport::Tuple& dest);
      
      void addBuffer(char* buf);

      // returns the encoded buffer which was encoded by resolve()
      // should only be called by the TransportSelector
      Data& getEncoded();

      UInt64 getCreatedTimeMicroSec() {return mCreatedTime;}

      // deal with a notion of an "out-of-band" forced target for SIP routing
      void setTarget(const Uri& uri);
      void clearTarget();
      const Uri& getTarget() const;
      bool hasTarget() const;
      
   private:
      void compute2543TransactionHash() const;
      
      void copyFrom(const SipMessage& message);
      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single) const;

      // not available
      SipMessage& operator=(const SipMessage&);

      bool mIsExternal;
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      mutable UnknownHeaders mUnknownHeaders;
  
      Transport::Tuple mSource;
      Transport::Tuple mDestination;

      std::vector<char*> mBufferList;
      mutable HeaderFieldValueList* mStartLine;
      mutable HeaderFieldValue* mContentsHfv;
      mutable Contents* mContents;
      mutable Data mRFC2543TransactionId;
      mutable bool mRequest;
      mutable bool mResponse;

      Data mEncoded; // to be retransmitted
      UInt64 mCreatedTime;

      Uri* mTarget;
      
      friend class TransportSelector;
};

}

#undef ensureHeaderTypeUseable
#undef ensureSingleHeader
#undef ensureMultiHeader

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

#endif
