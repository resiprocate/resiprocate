#include "sip2/sipstack/Contents.hxx"
#include "sip2/sipstack/HeaderFieldValueList.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/CountStream.hxx"
#include "sip2/util/MD5Stream.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/compat.hxx"
#include "sip2/util/vmd5.hxx"
#include "sip2/util/Coders.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

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
      assert (!header(h_Vias).front().param(p_branch).transactionId().empty());
      return header(h_Vias).front().param(p_branch).transactionId();
   }
   else
   {

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

      if( mRFC2543TransactionId.empty() )
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
         
   
         strm << header(h_CallId).value();

         if (header(h_RequestLine).getMethod() == ACK)
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

         // Allows incremental computation of the transaction hash. When the
         // response comes back from the server, the Transaction needs to add a
         // new entry to the TransactionMap 
         if (header(h_To).exists(p_tag) && header(h_RequestLine).getMethod() != INVITE)
         {
            MD5Stream strm2;
            strm2 << mRFC2543TransactionId
                  << header(h_To).param(p_tag);
            mRFC2543TransactionId = strm2.getHex();
         }
      }
      return mRFC2543TransactionId;
   }
}

void
SipMessage::copyRFC2543TransactionId(const SipMessage& request)
{
   assert(isResponse());
   mRFC2543TransactionId = request.mRFC2543TransactionId;
}

const Data&
SipMessage::updateRFC2543TransactionId()
{
   assert(isResponse());
   MD5Stream strm2;
   strm2 << mRFC2543TransactionId
         << header(h_To).param(p_tag);
   mRFC2543TransactionId = strm2.getHex();
   return mRFC2543TransactionId;
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
   Data result;
   
   if (isRequest()) 
   {
      result += "SipRequest: ";
      MethodTypes meth = header(h_RequestLine).getMethod();
      result += (meth != UNKNOWN) ? MethodNames[meth] : Data( "UNKNOWN" );
      result += Data( " " );
      result += header(h_RequestLine).uri().getAor();
   }
   else if (isResponse())
   {
      result += "SipResponse: ";
      result += Data(header(h_StatusLine).responseCode());
   }
   result += " cseq=";
   result += MethodNames[header(h_CSeq).method()];
   result += " / ";
   result += Data(header(h_CSeq).sequence());
   result += mIsExternal ? " from(wire)" : " from(TU)";
   
   return result;
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
      if (i != Headers::Content_Length) // !dlb! hack...
      {
         if (mHeaders[i] != 0)
         {
            mHeaders[i]->encode(Headers::HeaderNames[static_cast<Headers::Type>(i)], str);
         }
      }
      else
      {
         if (mContents != 0)
         {
            mContents->mHeadersFromMessage = true;

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
      if (i != Headers::Content_Length)
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
            mHeaders[i]->encodeEmbedded(Headers::HeaderNames[static_cast<Headers::Type>(i)], str);
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
   delete mContentsHfv;
   mContentsHfv = 0;
   
   mContents = contents->clone();

   // copy contents headers into message
   header(h_ContentType) = contents->getType();
   if (mContents->exists(h_ContentDisposition))
   {
      header(h_ContentDisposition) = mContents->header(h_ContentDisposition);
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
   if (mContents == 0)
   {
      if (!exists(h_ContentType))
      {
         DebugLog(<< "SipMessage::getContents: ContentType header does not exist");
         return 0;
      }
      DebugLog(<< "SipMessage::getContents: " << header(h_ContentType));
      assert(Contents::getFactoryMap().find(header(h_ContentType)) != Contents::getFactoryMap().end());
      mContents = Contents::getFactoryMap()[header(h_ContentType)]->create(mContentsHfv, header(h_ContentType));
      // copy contents headers into the contents
      if (exists(h_ContentDisposition))
      {
         mContents->header(h_ContentDisposition) = header(h_ContentDisposition);
      }
      if (exists(h_ContentLanguages))
      {
         mContents->header(h_ContentLanguages) = header(h_ContentLanguages);
      }
      if (exists(h_ContentType))
      {
         mContents->header(h_ContentType) = header(h_ContentType);
      }
      // contents' headers from message
      mContents->mHeadersFromMessage = true;
   }
   return mContents;
}

// unknown header interface
StringCategories& 
SipMessage::header(const Data& headerName) const
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == headerName)
      {
         HeaderFieldValueList* hfvs = i->second;
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::NONE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }

   // !dlb! backward compatibility -- Unknown header becomes a known header
   // lookup known header of same name
   // if found, convert to Unknown header and remove as known header

   // create the list empty
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::NONE));
   mUnknownHeaders.push_back(make_pair(headerName, hfvs));
   return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
}

bool
SipMessage::exists(const Data& symbol) const
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == symbol)
      {
         return true;
      }
   }
   return false;
}

void
SipMessage::remove(const Data& headerName)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == headerName)
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
   HeaderFieldValue* newHeader = new HeaderFieldValue(start, len);
   
   if (header != Headers::UNKNOWN)
   {
      if (mHeaders[header] == 0)
      {
         mHeaders[header] = new HeaderFieldValueList;
         mHeaders[header]->push_back(newHeader);
      }
      else
      {
         mHeaders[header]->push_back(newHeader);
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
            i->second->push_back(newHeader);
            return;
         }
      }
      // didn't find it, add an entry
      HeaderFieldValueList *hfvs = new HeaderFieldValueList();
      hfvs->push_back(newHeader);
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
SipMessage::ensureHeaders(Headers::Type type) const
{
   HeaderFieldValueList* hfvs = mHeaders[type];
   
   // empty?
   if (hfvs == 0)
   {
      // create the list
      hfvs = new HeaderFieldValueList;
      mHeaders[type] = hfvs;
   }
   return hfvs;
}

HeaderFieldValueList* 
SipMessage::ensureHeader(Headers::Type type) const
{
   HeaderFieldValueList* hfvs = mHeaders[type];
   
   // empty?
   if (hfvs == 0)
   {
      // create the list with a new component
      hfvs = new HeaderFieldValueList;
      HeaderFieldValue* hfv = new HeaderFieldValue;
      hfvs->push_back(hfv);
      mHeaders[type] = hfvs;
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

CSeq_Header::Type&
SipMessage::header(const CSeq_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<CSeq_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<CSeq_Header::Type>*>(hfvs->getParserContainer())->front();
};

Call_ID_Header::Type&
SipMessage::header(const Call_ID_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Call_ID_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Call_ID_Header::Type>*>(hfvs->getParserContainer())->front();
};

Authentication_Info_Header::Type&
SipMessage::header(const Authentication_Info_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authentication_Info_Header::Type>(hfvs,
                                                                                     headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Authentication_Info_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Disposition_Header::Type&
SipMessage::header(const Content_Disposition_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Disposition_Header::Type>(hfvs,
                                                                                     headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Content_Disposition_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Encoding_Header::Type&
SipMessage::header(const Content_Encoding_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Encoding_Header::Type>(hfvs,
                                                                                  headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Content_Encoding_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Length_Header::Type&
SipMessage::header(const Content_Length_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Length_Header::Type>(hfvs,
                                                                                headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Content_Length_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Type_Header::Type&
SipMessage::header(const Content_Type_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Type_Header::Type>(hfvs,
                                                                              headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Content_Type_Header::Type>*>(hfvs->getParserContainer())->front();
};

Date_Header::Type&
SipMessage::header(const Date_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Date_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Date_Header::Type>*>(hfvs->getParserContainer())->front();
};

Event_Header::Type&
SipMessage::header(const Event_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Event_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Event_Header::Type>*>(hfvs->getParserContainer())->front();
};

Expires_Header::Type&
SipMessage::header(const Expires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Expires_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Expires_Header::Type>*>(hfvs->getParserContainer())->front();
};

From_Header::Type&
SipMessage::header(const From_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<From_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<From_Header::Type>*>(hfvs->getParserContainer())->front();
};

In_Reply_To_Header::Type&
SipMessage::header(const In_Reply_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<In_Reply_To_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<In_Reply_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

MIME_Version_Header::Type&
SipMessage::header(const MIME_Version_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<MIME_Version_Header::Type>(hfvs,
                                                                              headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<MIME_Version_Header::Type>*>(hfvs->getParserContainer())->front();
};

Max_Forwards_Header::Type&
SipMessage::header(const Max_Forwards_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Max_Forwards_Header::Type>(hfvs,
                                                                              headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Max_Forwards_Header::Type>*>(hfvs->getParserContainer())->front();
};

Min_Expires_Header::Type&
SipMessage::header(const Min_Expires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Min_Expires_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Min_Expires_Header::Type>*>(hfvs->getParserContainer())->front();
};

Organization_Header::Type&
SipMessage::header(const Organization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
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
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Priority_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Priority_Header::Type>*>(hfvs->getParserContainer())->front();
};

Refer_To_Header::Type&
SipMessage::header(const Refer_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Refer_To_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Refer_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

Referred_By_Header::Type&
SipMessage::header(const Referred_By_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Referred_By_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Referred_By_Header::Type>*>(hfvs->getParserContainer())->front();
};

Replaces_Header::Type&
SipMessage::header(const Replaces_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Replaces_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Replaces_Header::Type>*>(hfvs->getParserContainer())->front();
};

Reply_To_Header::Type&
SipMessage::header(const Reply_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Reply_To_Header::Type>(hfvs,
                                                                          headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Reply_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

Retry_After_Header::Type&
SipMessage::header(const Retry_After_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Retry_After_Header::Type>(hfvs, headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Retry_After_Header::Type>*>(hfvs->getParserContainer())->front();
};

Server_Header::Type&
SipMessage::header(const Server_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
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
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
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
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
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
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<To_Header::Type>(hfvs,
                                                                    headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<To_Header::Type>*>(hfvs->getParserContainer())->front();
};

User_Agent_Header::Type&
SipMessage::header(const User_Agent_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<User_Agent_Header::Type>(hfvs,
                                                                            headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<User_Agent_Header::Type>*>(hfvs->getParserContainer())->front();
};

Warning_Header::Type&
SipMessage::header(const Warning_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Warning_Header::Type>(hfvs,
                                                                         headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Warning_Header::Type>*>(hfvs->getParserContainer())->front();
};

ParserContainer<Security_Client_MultiHeader::Type>&
SipMessage::header(const Security_Client_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Security_Client_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Security_Client_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Security_Server_MultiHeader::Type>&
SipMessage::header(const Security_Server_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Security_Server_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Security_Server_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Security_Verify_MultiHeader::Type>&
SipMessage::header(const Security_Verify_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Security_Verify_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Security_Verify_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Authorization_MultiHeader::Type>&
SipMessage::header(const Authorization_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authorization_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Authorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Proxy_Authenticate_MultiHeader::Type>&
SipMessage::header(const Proxy_Authenticate_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authenticate_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Proxy_Authorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Proxy_Authorization_MultiHeader::Type>&
SipMessage::header(const Proxy_Authorization_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authorization_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Proxy_Authorization_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<WWW_Authenticate_MultiHeader::Type>&
SipMessage::header(const WWW_Authenticate_MultiHeader& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<WWW_Authenticate_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<WWW_Authenticate_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_MultiHeader::Type>&
SipMessage::header(const Accept_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Accept_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_Encoding_MultiHeader::Type>&
SipMessage::header(const Accept_Encoding_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_Encoding_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Accept_Encoding_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_Language_MultiHeader::Type>&
SipMessage::header(const Accept_Language_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_Language_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Accept_Language_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Alert_Info_MultiHeader::Type>&
SipMessage::header(const Alert_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Alert_Info_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Alert_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Allow_MultiHeader::Type>&
SipMessage::header(const Allow_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Allow_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Allow_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Call_Info_MultiHeader::Type>&
SipMessage::header(const Call_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Call_Info_MultiHeader::Type>(hfvs,
                                                                                headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Call_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Contact_MultiHeader::Type>&
SipMessage::header(const Contact_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Contact_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Contact_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Content_Language_MultiHeader::Type>&
SipMessage::header(const Content_Language_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Language_MultiHeader::Type>(hfvs,
                                                                                       headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Content_Language_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Error_Info_MultiHeader::Type>&
SipMessage::header(const Error_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Error_Info_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Error_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Proxy_Require_MultiHeader::Type>&
SipMessage::header(const Proxy_Require_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Require_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Proxy_Require_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Record_Route_MultiHeader::Type>&
SipMessage::header(const Record_Route_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Record_Route_MultiHeader::Type>(hfvs,
                                                                                   headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Record_Route_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Require_MultiHeader::Type>&
SipMessage::header(const Require_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Require_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Require_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Route_MultiHeader::Type>&
SipMessage::header(const Route_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Route_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Route_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Subscription_State_MultiHeader::Type>&
SipMessage::header(const Subscription_State_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Subscription_State_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Subscription_State_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Supported_MultiHeader::Type>&
SipMessage::header(const Supported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Supported_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Supported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Unsupported_MultiHeader::Type>&
SipMessage::header(const Unsupported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Unsupported_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Unsupported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Via_MultiHeader::Type>&
SipMessage::header(const Via_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Via_MultiHeader::Type>(hfvs, headerType.getTypeNum()));
   }
   return *dynamic_cast<ParserContainer<Via_MultiHeader::Type>*>(hfvs->getParserContainer());
};

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
namespace Vocal2
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
