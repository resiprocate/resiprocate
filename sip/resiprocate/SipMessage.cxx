#include "resiprocate/Contents.hxx"
#include "resiprocate/OctetContents.hxx"
#include "resiprocate/HeaderFieldValueList.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/UnknownHeaderType.hxx"
#include "resiprocate/os/Coders.hxx"
#include "resiprocate/os/CountStream.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/vmd5.hxx"
#include "resiprocate/os/Coders.hxx"
#include "resiprocate/os/Random.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const SipMessage::FromWireType* SipMessage::FromWire = new SipMessage::FromWireType();
const SipMessage::FromWireType* SipMessage::NotFromWire = new SipMessage::FromWireType();

SipMessage::SipMessage(const FromWireType* fromWire)
   : mIsExternal(fromWire == SipMessage::FromWire),
     mStartLine(0),
     mContentsHfv(0),
     mContents(0),
     mRFC2543TransactionId(),
     mRequest(false),
     mResponse(false),
     mCreatedTime(Timer::getTimeMicroSec()),
     mTarget(0)
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      mHeaders[i] = 0;
   }
}

SipMessage::SipMessage(const SipMessage& from)
   : mIsExternal(from.mIsExternal),
     mSource(from.mSource),
     mDestination(from.mDestination),
     mStartLine(0),
     mContentsHfv(0),
     mContents(0),
     mRFC2543TransactionId(from.mRFC2543TransactionId),
     mRequest(from.mRequest),
     mResponse(from.mResponse),
     mCreatedTime(Timer::getTimeMicroSec()),
     mTarget(0)
{
   if (this != &from)
   {
      for (int i = 0; i < Headers::MAX_HEADERS; i++)
      {
         if (from.mHeaders[i] != 0)
         {
            mHeaders[i] = new HeaderFieldValueList(*from.mHeaders[i]);
         }
         else
         {
            mHeaders[i] = 0;
         }
      }
  
      for (UnknownHeaders::const_iterator i = from.mUnknownHeaders.begin();
           i != from.mUnknownHeaders.end(); i++)
      {
         mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(
                                      i->first,
                                      new HeaderFieldValueList(*i->second)));
      }
      if (from.mStartLine != 0)
      {
         mStartLine = new HeaderFieldValueList(*from.mStartLine); 
      }
      if (from.mContents != 0)
      {
         mContents = from.mContents->clone();
      }
      else if (from.mContentsHfv != 0)
      {
         mContentsHfv = new HeaderFieldValue(*from.mContentsHfv);
      }
      else
      {
         // no body to copy
      }
      if (from.mTarget != 0)
      {
	 mTarget = new Uri(*from.mTarget);
      }
   }
}

SipMessage::~SipMessage()
{
   //DebugLog (<< "Deleting SipMessage: " << brief());
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      delete mHeaders[i];
   }

   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      delete i->second;
   }
   
   for (vector<char*>::iterator i = mBufferList.begin();
        i != mBufferList.end(); i++)
   {
      delete [] *i;
   }
   
   delete mStartLine;
   delete mContents;
   delete mContentsHfv;
   delete mTarget;
}

const Data& 
SipMessage::getTransactionId() const
{
   assert (!header(h_Vias).empty());
   if( header(h_Vias).front().exists(p_branch) 
       && header(h_Vias).front().param(p_branch).hasMagicCookie() )
   {
      assert (!header(h_Vias).front().param(p_branch).getTransactionId().empty());
      return header(h_Vias).front().param(p_branch).getTransactionId();
   }
   else
   {
      if (mRFC2543TransactionId.empty())
      {
         compute2543TransactionHash();
      }
      return mRFC2543TransactionId;
   }
}

void
SipMessage::compute2543TransactionHash() const
{
   assert (mRFC2543TransactionId.empty());
   
   /*  From rfc3261, 17.2.3
       The INVITE request matches a transaction if the Request-URI, To tag,
       From tag, Call-ID, CSeq, and top Via header field match those of the
       INVITE request which created the transaction.  In this case, the
       INVITE is a retransmission of the original one that created the
       transaction.  

       The ACK request matches a transaction if the Request-URI, From tag,
       Call-ID, CSeq number (not the method), and top Via header field match
       those of the INVITE request which created the transaction, and the To
       tag of the ACK matches the To tag of the response sent by the server
       transaction.  

       Matching is done based on the matching rules defined for each of those
       header fields.  Inclusion of the tag in the To header field in the ACK
       matching process helps disambiguate ACK for 2xx from ACK for other
       responses at a proxy, which may have forwarded both responses (This
       can occur in unusual conditions.  Specifically, when a proxy forked a
       request, and then crashes, the responses may be delivered to another
       proxy, which might end up forwarding multiple responses upstream).  An
       ACK request that matches an INVITE transaction matched by a previous
       ACK is considered a retransmission of that previous ACK.

       For all other request methods, a request is matched to a transaction
       if the Request-URI, To tag, From tag, Call-ID, CSeq (including the
       method), and top Via header field match those of the request that
       created the transaction.  Matching is done based on the matching
   */

   // If it is here and isn't a request, leave the transactionId empty, this
   // will cause the Transaction to send it statelessly

   if (isRequest())
   {
      MD5Stream strm;
      // See section 17.2.3 Matching Requests to Server Transactions in rfc 3261
         
      strm << header(h_RequestLine).uri().scheme();
      strm << header(h_RequestLine).uri().user();
      strm << header(h_RequestLine).uri().host();
      strm << header(h_RequestLine).uri().port();
      strm << header(h_RequestLine).uri().password();
      strm << header(h_RequestLine).uri().commutativeParameterHash();

      if (!header(h_Vias).empty())
      {
         strm << header(h_Vias).front().protocolName();
         strm << header(h_Vias).front().protocolVersion();
         strm << header(h_Vias).front().transport();
         strm << header(h_Vias).front().sentHost();
         strm << header(h_Vias).front().sentPort();
         strm << header(h_Vias).front().commutativeParameterHash();
      }
         
      if (header(h_From).exists(p_tag))
      {
         strm << header(h_From).param(p_tag);
      }

      // Only include the totag for non-invite requests
      if (header(h_RequestLine).getMethod() != INVITE && 
          header(h_RequestLine).getMethod() != ACK && 
          header(h_RequestLine).getMethod() != CANCEL &&
          header(h_To).exists(p_tag))
      {
         strm << header(h_To).param(p_tag);
      }

      strm << header(h_CallId).value();

      if (header(h_RequestLine).getMethod() == ACK || 
          header(h_RequestLine).getMethod() == CANCEL)
      {
         strm << INVITE;
         strm << header(h_CSeq).sequence();
      }
      else
      {
         strm << header(h_CSeq).method();
         strm << header(h_CSeq).sequence();
      }
           
      mRFC2543TransactionId = strm.getHex();
   }
}

const Data&
SipMessage::getRFC2543TransactionId() const
{
   if (mRFC2543TransactionId.empty())
   {
      compute2543TransactionHash();
   }
   return mRFC2543TransactionId;
}

void
SipMessage::setRFC2543TransactionId(const Data& tid)
{
   mRFC2543TransactionId = tid;
}

bool
SipMessage::isRequest() const
{
   return mRequest;
}

bool
SipMessage::isResponse() const
{
   return mResponse;
}

Data
SipMessage::brief() const
{
   Data result(128, true);
   static const Data request("SipRequest: ");
   static const Data response("SipResponse: ");
   static const Data tid(" tid=");
   static const Data cseq(" cseq=");
   static const Data slash(" / ");
   static const Data wire(" from(wire)");
   static const Data tu(" from(tu)");
   
   if (isRequest()) 
   {
      result += request;
      MethodTypes meth = header(h_RequestLine).getMethod();
      result += MethodNames[meth];
      result += Symbols::SPACE;
      result += header(h_RequestLine).uri().getAor();
   }
   else if (isResponse())
   {
      result += response;
      result += Data(header(h_StatusLine).responseCode());
   }
   result += tid;
   result += getTransactionId();
   result += cseq;
   result += MethodNames[header(h_CSeq).method()];
   result += slash;
   result += Data(header(h_CSeq).sequence());
   result += mIsExternal ? wire : tu;
   
   return result;
}

bool
SipMessage::isClientTransaction() const
{
   assert(mRequest || mResponse);
   return ((mIsExternal && mResponse) || (!mIsExternal && mRequest));
}


// dynamic_cast &str to DataStream* to avoid CountStream?

std::ostream& 
SipMessage::encode(std::ostream& str) const
{
   if (mStartLine != 0)
   {
      mStartLine->encode(Data::Empty, str);
   }

   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (i != Headers::ContentLength) // !dlb! hack...
      {
         if (mHeaders[i] != 0)
         {
            mHeaders[i]->encode(Headers::getHeaderName(i), str);
         }
      }
      else
      {
         if (mContents != 0)
         {
            CountStream cs;
            mContents->encode(cs);
            cs.flush();
            str << "Content-Length: " << cs.size() << "\r\n";
         }
         else if (mContentsHfv != 0)
         {
            str << "Content-Length: " << mContentsHfv->mFieldLength << "\r\n";
         }
         else
         {
            str << "Content-Length: 0\r\n";
         }
      }
   }

   for (UnknownHeaders::const_iterator i = mUnknownHeaders.begin(); 
        i != mUnknownHeaders.end(); i++)
   {
      i->second->encode(i->first, str);
   }

   str << Symbols::CRLF;
   
   if (mContents != 0)
   {
      mContents->encode(str);
   }
   else if (mContentsHfv != 0)
   {
      mContentsHfv->encode(str);
   }
   
   return str;
}

std::ostream& 
SipMessage::encodeEmbedded(std::ostream& str) const
{
   bool first = true;
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (i != Headers::ContentLength)
      {
         if (mHeaders[i] != 0)
         {
            if (first)
            {
               str << Symbols::QUESTION;
               first = false;
            }
            else
            {
               str << Symbols::AMPERSAND;
            }
            mHeaders[i]->encodeEmbedded(Headers::getHeaderName(i), str);
         }
      }
   }

   for (UnknownHeaders::const_iterator i = mUnknownHeaders.begin(); 
        i != mUnknownHeaders.end(); i++)
   {
      if (first)
      {
         str << Symbols::QUESTION;
         first = false;
      }
      else
      {
         str << Symbols::AMPERSAND;
      }
      i->second->encodeEmbedded(i->first, str);
   }

   if (mContents != 0)
   {
      if (first)
      {
         str << Symbols::QUESTION;
      }
      else
      {
         str << Symbols::AMPERSAND;
      }
      str << "body=";
      // !dlb! encode escaped for characters
      Data contents;
      {
         DataStream s(contents);
         mContents->encode(s);
      }
      str << Embedded::encode(contents);
   }
   else if (mContentsHfv != 0)
   {
      if (first)
      {
         str << Symbols::QUESTION;
      }
      else
      {
         str << Symbols::AMPERSAND;
      }
      str << "body=";
      // !dlb! encode escaped for characters
      Data contents;
      {
         DataStream s(contents);
         mContentsHfv->encode(str);
      }
      str << Embedded::encode(contents);
   }
   
   return str;
}

void
SipMessage::addBuffer(char* buf)
{
   mBufferList.push_back(buf);
}

void 
SipMessage::setStartLine(const char* st, int len)
{
   mStartLine = new HeaderFieldValueList;
   mStartLine-> push_back(new HeaderFieldValue(st, len));
   ParseBuffer pb(st, len);
   const char* start;
   start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   MethodTypes method = getMethodType(start, pb.position() - start);
   if (method == UNKNOWN) //probably a status line
   {
      start = pb.skipChar(Symbols::SPACE[0]);
      pb.skipNonWhitespace();
      if ((pb.position() - start) == 3)
      {
         mStartLine->setParserContainer(new ParserContainer<StatusLine>(mStartLine, Headers::NONE));
         //!dcm! should invoke the statusline parser here once it does limited validation
         mResponse = true;
      }
   }
   if (!mResponse)
   {
      mStartLine->setParserContainer(new ParserContainer<RequestLine>(mStartLine, Headers::NONE));
      //!dcm! should invoke the responseline parser here once it does limited validation
      mRequest = true;
   }
}

void 
SipMessage::setBody(const char* start, int len)
{
   mContentsHfv = new HeaderFieldValue(start, len);
}

void 
SipMessage::setContents(const Contents* contents)
{
   delete mContents;
   mContents = 0;
   delete mContentsHfv;
   mContentsHfv = 0;

   if (contents == 0)
   {
      // The semantics of setContents(0) are to delete message contents
      return;
   }
 
   mContents = contents->clone();

   // copy contents headers into message
   header(h_ContentType) = contents->getType();
   if (mContents->exists(h_ContentDisposition))
   {
      header(h_ContentDisposition) = mContents->header(h_ContentDisposition);
   }
   if (mContents->exists(h_ContentTransferEncoding))
   {
      header(h_ContentTransferEncoding) = mContents->header(h_ContentTransferEncoding);
   }
   if (mContents->exists(h_ContentLanguages))
   {
      header(h_ContentLanguages) = mContents->header(h_ContentLanguages);
   }
   if (mContents->exists(h_ContentType))
   {
      header(h_ContentType) = mContents->header(h_ContentType);
      assert( header(h_ContentType).type() == mContents->getType().type() );
      assert( header(h_ContentType).subType() == mContents->getType().subType() );
   }
   else
   {
      header(h_ContentType) = mContents->getType();
   }
}

Contents*
SipMessage::getContents() const
{
   if (mContents == 0 && mContentsHfv != 0)
   {
      if (!exists(h_ContentType))
      {
         DebugLog(<< "SipMessage::getContents: ContentType header does not exist");
         return 0;
      }
      DebugLog(<< "SipMessage::getContents: " << header(h_ContentType));

      if ( Contents::getFactoryMap().find(header(h_ContentType)) == Contents::getFactoryMap().end() )
      {
         InfoLog(<< "SipMessage::getContents: got content type ("
                 <<  header(h_ContentType) << ") that is not known, "
                 << "returning as opaque application/octet-stream");
         mContents = Contents::getFactoryMap()[OctetContents::getStaticType()]->create(mContentsHfv, OctetContents::getStaticType());
      }
      else
      {
         mContents = Contents::getFactoryMap()[header(h_ContentType)]->create(mContentsHfv, header(h_ContentType));
      }
      
      // copy contents headers into the contents
      if (exists(h_ContentDisposition))
      {
         mContents->header(h_ContentDisposition) = header(h_ContentDisposition);
      }
      if (exists(h_ContentTransferEncoding))
      {
         mContents->header(h_ContentTransferEncoding) = header(h_ContentTransferEncoding);
      }
      if (exists(h_ContentLanguages))
      {
         mContents->header(h_ContentLanguages) = header(h_ContentLanguages);
      }
      if (exists(h_ContentType))
      {
         mContents->header(h_ContentType) = header(h_ContentType);
      }
      // !dlb! Content-Transfer-Encoding?
   }
   return mContents;
}

// unknown header interface
StringCategories& 
SipMessage::header(const UnknownHeaderType& headerName) const
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      // !dlb! case sensitive?
      if (i->first == headerName.getName())
      {
         HeaderFieldValueList* hfvs = i->second;
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::NONE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }

   // create the list empty
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::NONE));
   mUnknownHeaders.push_back(make_pair(headerName.getName(), hfvs));
   return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
}

bool
SipMessage::exists(const UnknownHeaderType& symbol) const
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == symbol.getName())
      {
         return true;
      }
   }
   return false;
}

void
SipMessage::remove(const UnknownHeaderType& headerName)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == headerName.getName())
      {
         delete i->second;
         mUnknownHeaders.erase(i);
         return;
      }
   }
}

void
SipMessage::addHeader(Headers::Type header, const char* headerName, int headerLen, 
                      const char* start, int len)
{
   if (header != Headers::UNKNOWN)
   {
      if (mHeaders[header] == 0)
      {
         mHeaders[header] = new HeaderFieldValueList;
      }
      if (len)
      {
         mHeaders[header]->push_back(new HeaderFieldValue(start, len));
      }
   }
   else
   {
      for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
           i != mUnknownHeaders.end(); i++)
      {
         if (strncasecmp(i->first.data(), headerName, headerLen) == 0)
         {
            // add to end of list
            if (len)
            {
               i->second->push_back(new HeaderFieldValue(start, len));
            }
            return;
         }
      }

      // didn't find it, add an entry
      HeaderFieldValueList *hfvs = new HeaderFieldValueList();
      if (len)
      {
         hfvs->push_back(new HeaderFieldValue(start, len));
      }
      mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(Data(headerName, headerLen),
                                                                  hfvs));
   }
}

Data&
SipMessage::getEncoded() 
{
   return mEncoded;
}

RequestLine& 
SipMessage::header(const RequestLineType& l) const
{
   assert (!isResponse());
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValueList;
      mStartLine->push_back(new HeaderFieldValue);
      mStartLine->setParserContainer(new ParserContainer<RequestLine>(mStartLine, Headers::NONE));
      mRequest = true;
   }
   return dynamic_cast<ParserContainer<RequestLine>*>(mStartLine->getParserContainer())->front();
}

StatusLine& 
SipMessage::header(const StatusLineType& l) const
{
   assert (!isRequest());
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValueList;
      mStartLine->push_back(new HeaderFieldValue);
      mStartLine->setParserContainer(new ParserContainer<StatusLine>(mStartLine, Headers::NONE));
      mResponse = true;
   }
   return dynamic_cast<ParserContainer<StatusLine>*>(mStartLine->getParserContainer())->front();
}

HeaderFieldValueList* 
SipMessage::ensureHeaders(Headers::Type type, bool single) const
{
   HeaderFieldValueList* hfvs = mHeaders[type];
   
   // empty?
   if (hfvs == 0)
   {
      // create the list with a new component
      hfvs = new HeaderFieldValueList;
      mHeaders[type] = hfvs;
      if (single)
      {
         HeaderFieldValue* hfv = new HeaderFieldValue;
         hfvs->push_back(hfv);
      }
   }
   // !dlb! not thrilled about checking this every access
   else if (single)
   {
      if (hfvs->empty())
      {
         // create an unparsed shared header field value
         hfvs->push_back(new HeaderFieldValue(Data::Empty.data(), 0));
      }
   }

   return hfvs;
}

// type safe header accessors
bool    
SipMessage::exists(const HeaderBase& headerType) const 
{
   return mHeaders[headerType.getTypeNum()] != 0;
};

void
SipMessage::remove(const HeaderBase& headerType)
{
   delete mHeaders[headerType.getTypeNum()]; 
   mHeaders[headerType.getTypeNum()] = 0; 
};

#ifndef PARTIAL_TEMPLATE_SPECIALIZATION
ParserContainer<AllowEvents_MultiHeader::Type>&
SipMessage::header(const AllowEvents_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<AllowEvents_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<AllowEvents_MultiHeader::Type>*>(hfvs->getParserContainer());
};

CSeq_Header::Type&
SipMessage::header(const CSeq_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<CSeq_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<CSeq_Header::Type>*>(hfvs->getParserContainer())->front();
};

CallId_Header::Type&
SipMessage::header(const CallId_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<CallId_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<CallId_Header::Type>*>(hfvs->getParserContainer())->front();
};

AuthenticationInfo_Header::Type&
SipMessage::header(const AuthenticationInfo_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<AuthenticationInfo_Header::Type>(hfvs,
                                                                                    headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<AuthenticationInfo_Header::Type>*>(hfvs->getParserContainer())->front();
};

ContentDisposition_Header::Type&
SipMessage::header(const ContentDisposition_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentDisposition_Header::Type>(hfvs,
                                                                                    headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ContentDisposition_Header::Type>*>(hfvs->getParserContainer())->front();
};

ContentTransferEncoding_Header::Type&
SipMessage::header(const ContentTransferEncoding_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentTransferEncoding_Header::Type>(hfvs,
                                                                                         headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ContentTransferEncoding_Header::Type>*>(hfvs->getParserContainer())->front();
};

ContentEncoding_Header::Type&
SipMessage::header(const ContentEncoding_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentEncoding_Header::Type>(hfvs,
                                                                                 headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ContentEncoding_Header::Type>*>(hfvs->getParserContainer())->front();
};

ContentLength_Header::Type&
SipMessage::header(const ContentLength_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentLength_Header::Type>(hfvs,
                                                                               headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ContentLength_Header::Type>*>(hfvs->getParserContainer())->front();
};

ContentType_Header::Type&
SipMessage::header(const ContentType_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentType_Header::Type>(hfvs,
                                                                             headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ContentType_Header::Type>*>(hfvs->getParserContainer())->front();
};

Date_Header::Type&
SipMessage::header(const Date_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Date_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Date_Header::Type>*>(hfvs->getParserContainer())->front();
};

Event_Header::Type&
SipMessage::header(const Event_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Event_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Event_Header::Type>*>(hfvs->getParserContainer())->front();
};

Expires_Header::Type&
SipMessage::header(const Expires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Expires_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Expires_Header::Type>*>(hfvs->getParserContainer())->front();
};

From_Header::Type&
SipMessage::header(const From_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<From_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<From_Header::Type>*>(hfvs->getParserContainer())->front();
};

InReplyTo_Header::Type&
SipMessage::header(const InReplyTo_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<InReplyTo_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<InReplyTo_Header::Type>*>(hfvs->getParserContainer())->front();
};

MIMEVersion_Header::Type&
SipMessage::header(const MIMEVersion_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<MIMEVersion_Header::Type>(hfvs,
                                                                             headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<MIMEVersion_Header::Type>*>(hfvs->getParserContainer())->front();
};

MaxForwards_Header::Type&
SipMessage::header(const MaxForwards_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<MaxForwards_Header::Type>(hfvs,
                                                                             headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<MaxForwards_Header::Type>*>(hfvs->getParserContainer())->front();
};

MinExpires_Header::Type&
SipMessage::header(const MinExpires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<MinExpires_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<MinExpires_Header::Type>*>(hfvs->getParserContainer())->front();
};

Organization_Header::Type&
SipMessage::header(const Organization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Organization_Header::Type>(hfvs,
                                                                              headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Organization_Header::Type>*>(hfvs->getParserContainer())->front();
};

Priority_Header::Type&
SipMessage::header(const Priority_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Priority_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Priority_Header::Type>*>(hfvs->getParserContainer())->front();
};

ReferTo_Header::Type&
SipMessage::header(const ReferTo_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ReferTo_Header::Type>(hfvs,
                                                                         headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ReferTo_Header::Type>*>(hfvs->getParserContainer())->front();
};

ReferredBy_Header::Type&
SipMessage::header(const ReferredBy_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ReferredBy_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ReferredBy_Header::Type>*>(hfvs->getParserContainer())->front();
};

Replaces_Header::Type&
SipMessage::header(const Replaces_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Replaces_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Replaces_Header::Type>*>(hfvs->getParserContainer())->front();
};

ReplyTo_Header::Type&
SipMessage::header(const ReplyTo_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ReplyTo_Header::Type>(hfvs,
                                                                         headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<ReplyTo_Header::Type>*>(hfvs->getParserContainer())->front();
};

RetryAfter_Header::Type&
SipMessage::header(const RetryAfter_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<RetryAfter_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<RetryAfter_Header::Type>*>(hfvs->getParserContainer())->front();
};

Server_Header::Type&
SipMessage::header(const Server_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Server_Header::Type>(hfvs,
                                                                        headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Server_Header::Type>*>(hfvs->getParserContainer())->front();
};

Subject_Header::Type&
SipMessage::header(const Subject_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Subject_Header::Type>(hfvs,
                                                                         headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Subject_Header::Type>*>(hfvs->getParserContainer())->front();
};

Timestamp_Header::Type&
SipMessage::header(const Timestamp_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Timestamp_Header::Type>(hfvs,
                                                                           headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Timestamp_Header::Type>*>(hfvs->getParserContainer())->front();
};

To_Header::Type&
SipMessage::header(const To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<To_Header::Type>(hfvs,
                                                                    headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<To_Header::Type>*>(hfvs->getParserContainer())->front();
};

UserAgent_Header::Type&
SipMessage::header(const UserAgent_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<UserAgent_Header::Type>(hfvs,
                                                                           headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<UserAgent_Header::Type>*>(hfvs->getParserContainer())->front();
};

Warning_Header::Type&
SipMessage::header(const Warning_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Warning_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Warning_Header::Type>*>(hfvs->getParserContainer())->front();
};

ParserContainer<SecurityClient_MultiHeader::Type>&
SipMessage::header(const SecurityClient_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<SecurityClient_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<SecurityClient_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<SecurityServer_MultiHeader::Type>&
SipMessage::header(const SecurityServer_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<SecurityServer_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<SecurityServer_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<SecurityVerify_MultiHeader::Type>&
SipMessage::header(const SecurityVerify_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<SecurityVerify_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<SecurityVerify_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Authorization_MultiHeader::Type>&
SipMessage::header(const Authorization_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authorization_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Authorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<ProxyAuthenticate_MultiHeader::Type>&
SipMessage::header(const ProxyAuthenticate_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ProxyAuthenticate_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<ProxyAuthorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<ProxyAuthorization_MultiHeader::Type>&
SipMessage::header(const ProxyAuthorization_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ProxyAuthorization_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<ProxyAuthorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<WWWAuthenticate_MultiHeader::Type>&
SipMessage::header(const WWWAuthenticate_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<WWWAuthenticate_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<WWWAuthenticate_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_MultiHeader::Type>&
SipMessage::header(const Accept_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Accept_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<AcceptEncoding_MultiHeader::Type>&
SipMessage::header(const AcceptEncoding_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<AcceptEncoding_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<AcceptEncoding_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<AcceptLanguage_MultiHeader::Type>&
SipMessage::header(const AcceptLanguage_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<AcceptLanguage_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<AcceptLanguage_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<AlertInfo_MultiHeader::Type>&
SipMessage::header(const AlertInfo_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<AlertInfo_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<AlertInfo_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Allow_MultiHeader::Type>&
SipMessage::header(const Allow_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Allow_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Allow_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<CallInfo_MultiHeader::Type>&
SipMessage::header(const CallInfo_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<CallInfo_MultiHeader::Type>(hfvs,
                                                                               headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<CallInfo_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Contact_MultiHeader::Type>&
SipMessage::header(const Contact_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Contact_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Contact_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<ContentLanguage_MultiHeader::Type>&
SipMessage::header(const ContentLanguage_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ContentLanguage_MultiHeader::Type>(hfvs,
                                                                                      headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<ContentLanguage_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<ErrorInfo_MultiHeader::Type>&
SipMessage::header(const ErrorInfo_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ErrorInfo_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<ErrorInfo_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<ProxyRequire_MultiHeader::Type>&
SipMessage::header(const ProxyRequire_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<ProxyRequire_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<ProxyRequire_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<RecordRoute_MultiHeader::Type>&
SipMessage::header(const RecordRoute_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<RecordRoute_MultiHeader::Type>(hfvs,
                                                                                  headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<RecordRoute_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Require_MultiHeader::Type>&
SipMessage::header(const Require_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Require_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Require_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Route_MultiHeader::Type>&
SipMessage::header(const Route_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Route_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Route_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<SubscriptionState_MultiHeader::Type>&
SipMessage::header(const SubscriptionState_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<SubscriptionState_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<SubscriptionState_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Supported_MultiHeader::Type>&
SipMessage::header(const Supported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Supported_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Supported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Unsupported_MultiHeader::Type>&
SipMessage::header(const Unsupported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Unsupported_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Unsupported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Via_MultiHeader::Type>&
SipMessage::header(const Via_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Via_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Via_MultiHeader::Type>*>(hfvs->getParserContainer());
};
#endif

const HeaderFieldValueList*
SipMessage::getRawHeader(Headers::Type headerType) const
{
   return mHeaders[headerType];
}

void
SipMessage::setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType)
{
   if (mHeaders[headerType] != hfvs)
   {
      delete mHeaders[headerType];
      mHeaders[headerType] = new HeaderFieldValueList(*hfvs);
   }
}

void
SipMessage::setTarget(const Uri& uri)
{
   if (mTarget)
   {
      DebugLog(<< "SipMessage::setTarget: replacing forced target");
      *mTarget = uri;
   }
   else
   {
      mTarget = new Uri(uri);
   }
}

void
SipMessage::clearTarget()
{
   delete mTarget;
   mTarget = 0;
}

const Uri&
SipMessage::getTarget() const
{
   assert(mTarget);
   return *mTarget;
}

bool
SipMessage::hasTarget() const
{
   return (mTarget != 0);
}

#if defined(DEBUG) && defined(DEBUG_MEMORY)
namespace resip
{

void*
operator new(size_t size)
{
   void * p = std::operator new(size);
   DebugLog(<<"operator new | " << hex << p << " | "
            << dec << size);
   if (size == 60)
   {
      3;
   }
   
   return p;
}

void operator delete(void* p)
{
   DebugLog(<<"operator delete | " << hex << p << dec);
   return std::operator delete( p );
}

void operator delete[](void* p)
{
   DebugLog(<<"operator delete [] | " << hex << p << dec);
   return std::operator delete[] ( p );
}
 
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
