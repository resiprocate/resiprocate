#if !defined(RESIP_SIPMESSAGE_HXX)
#define RESIP_SIPMESSAGE_HXX 

#include "resiprocate/os/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <list>
#include <vector>
#include <utility>

#include "resiprocate/Headers.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/ParserContainer.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Timer.hxx"

namespace resip
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

      const Data& getRFC2543TransactionId() const;
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

      virtual bool isClientTransaction() const;
      
      virtual std::ostream& encode(std::ostream& str) const;
      std::ostream& encodeEmbedded(std::ostream& str) const;
      
      Data brief() const;

      bool isRequest() const;
      bool isResponse() const;

      const RequestLine& 
      header(const RequestLineType& l) const;

      RequestLine& 
      header(const RequestLineType& l);

      const StatusLine& 
      header(const StatusLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l);

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

#ifdef PARTIAL_TEMPLATE_SPECIALIZATION

      template <class T>
      typename T::UnknownReturn&
      SipMessage::header(const T& headerType)
      {
         HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), T::Single);
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<typename T::Type>(hfvs, headerType.getTypeNum()));
         }
         return T::knownReturn(hfvs->getParserContainer());
      };

      // looks identical, but it isn't -- ensureHeaders CONST called -- may throw
      template <class T>
      const typename T::UnknownReturn&
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

#define defineHeader(_header)                                                           \
      const _header##_Header::Type& header(const _header##_Header& headerType) const;   \
      _header##_Header::Type& header(const _header##_Header& headerType)

#define defineMultiHeader(_header)                                                                                      \
      const ParserContainer<_header##_MultiHeader::Type>& header(const _header##_MultiHeader& headerType) const;        \
      ParserContainer<_header##_MultiHeader::Type>& header(const _header##_MultiHeader& headerType)

      defineHeader(CSeq);
      defineHeader(CallId);
      defineHeader(AuthenticationInfo);
      defineHeader(ContentDisposition);
      defineHeader(ContentTransferEncoding);
      defineHeader(ContentEncoding);
      defineHeader(ContentLength);
      defineHeader(ContentType);
      defineHeader(Date);
      defineHeader(Event);
      defineHeader(Expires);
      defineHeader(From);
      defineHeader(InReplyTo);
      defineHeader(MIMEVersion);
      defineHeader(MaxForwards);
      defineHeader(MinExpires);
      defineHeader(Organization);
      defineHeader(Priority);
      defineHeader(ReferTo);
      defineHeader(ReferredBy);
      defineHeader(Replaces);
      defineHeader(ReplyTo);
      defineHeader(RetryAfter);
      defineHeader(Server);
      defineHeader(Subject);
      defineHeader(Timestamp);
      defineHeader(To);
      defineHeader(UserAgent);
      defineHeader(Warning);

      defineMultiHeader(SecurityClient);
      defineMultiHeader(SecurityServer);
      defineMultiHeader(SecurityVerify);

      defineMultiHeader(Authorization);
      defineMultiHeader(ProxyAuthenticate);
      defineMultiHeader(WWWAuthenticate);
      defineMultiHeader(ProxyAuthorization);

      defineMultiHeader(Accept);
      defineMultiHeader(AcceptEncoding);
      defineMultiHeader(AcceptLanguage);
      defineMultiHeader(AlertInfo);
      defineMultiHeader(Allow);
      defineMultiHeader(AllowEvents);
      defineMultiHeader(CallInfo);
      defineMultiHeader(Contact);
      defineMultiHeader(ContentLanguage);
      defineMultiHeader(ErrorInfo);
      defineMultiHeader(ProxyRequire);
      defineMultiHeader(RecordRoute);
      defineMultiHeader(Require);
      defineMultiHeader(Route);
      defineMultiHeader(SubscriptionState);
      defineMultiHeader(Supported);
      defineMultiHeader(Unsupported);
      defineMultiHeader(Via);

#endif // METHOD_TEMPLATES

      // unknown header interface
      const StringCategories& header(const UnknownHeaderType& symbol) const;
      StringCategories& header(const UnknownHeaderType& symbol);
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

      // Returns the source tuple that the message was received from
      // only makes sense for messages received from the wire
      void setSource(const Transport::Tuple& tuple) { mSource = tuple; }
      const Transport::Tuple& getSource() const { return mSource; }

      // Used by the stateless interface to specify where to send a request/response
      void setDestination(const Transport::Tuple& tuple) { mDestination = tuple; }
      const Transport::Tuple& getDestination() const { return mDestination; }

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

      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single);
      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single) const; // throws if not present

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
