#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Contents.hxx"
#include "resip/stack/Embedded.hxx"
#include "resip/stack/OctetContents.hxx"
#include "resip/stack/HeaderFieldValueList.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "rutil/Coders.hxx"
#include "rutil/CountStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/compat.hxx"
#include "rutil/vmd5.hxx"
#include "rutil/Coders.hxx"
#include "rutil/Random.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

bool SipMessage::checkContentLength=true;

SipMessage::SipMessage(const Transport* fromWire)
   : mIsBadAck200(false),
      mIsExternal(fromWire != 0),
     mTransport(fromWire),
     mStartLine(0),
     mContentsHfv(0),
     mContents(0),
     mRFC2543TransactionId(),
     mRequest(false),
     mResponse(false),
     mInvalid(false),
     mCreatedTime(Timer::getTimeMicroSec()),
     mForceTarget(0),
     mTlsDomain(Data::Empty)
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      mHeaders[i] = 0;
   }
}

SipMessage::SipMessage(const SipMessage& from)
   : mStartLine(0),
     mContentsHfv(0),
     mContents(0),
     mCreatedTime(Timer::getTimeMicroSec()),
     mForceTarget(0)
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      mHeaders[i] = 0;
   }

   *this = from;
}

Message*
SipMessage::clone() const
{
   return new SipMessage(*this);
}

SipMessage& 
SipMessage::operator=(const SipMessage& rhs)
{
   if (this != &rhs)
   {
      this->cleanUp();

      mIsBadAck200 = rhs.mIsBadAck200;
      mIsExternal = rhs.mIsExternal;
      mTransport = rhs.mTransport;
      mSource = rhs.mSource;
      mDestination = rhs.mDestination;
      mStartLine = 0;
      mContentsHfv = 0;
      mContents = 0;
      mRFC2543TransactionId = rhs.mRFC2543TransactionId;
      mRequest = rhs.mRequest;
      mResponse = rhs.mResponse;
      mInvalid = rhs.mInvalid;
      mReason = rhs.mReason;
      mForceTarget = 0;
      mTlsDomain = rhs.mTlsDomain;
      
      for (int i = 0; i < Headers::MAX_HEADERS; i++)
      {
         if (rhs.mHeaders[i] != 0)
         {
            mHeaders[i] = new HeaderFieldValueList(*rhs.mHeaders[i]);
         }
         else
         {
            mHeaders[i] = 0;
         }
      }
  
      for (UnknownHeaders::const_iterator i = rhs.mUnknownHeaders.begin();
           i != rhs.mUnknownHeaders.end(); i++)
      {
         mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(
                                      i->first,
                                      new HeaderFieldValueList(*i->second)));
      }
      if (rhs.mStartLine != 0)
      {
         mStartLine = new HeaderFieldValueList(*rhs.mStartLine); 
      }
      if (rhs.mContents != 0)
      {
         mContents = rhs.mContents->clone();
      }
      else if (rhs.mContentsHfv != 0)
      {
         mContentsHfv = new HeaderFieldValue(*rhs.mContentsHfv, HeaderFieldValue::CopyPadding);
      }
      else
      {
         // no body to copy
      }
      if (rhs.mForceTarget != 0)
      {
         mForceTarget = new Uri(*rhs.mForceTarget);
      }

      if (rhs.mSecurityAttributes.get())
      {

         if (!mSecurityAttributes.get())
         {
            SecurityAttributes* attr = new SecurityAttributes();
            mSecurityAttributes.reset(attr);
         }

         if (rhs.mSecurityAttributes->isEncrypted())
         {
            mSecurityAttributes->setEncrypted();
         }
         mSecurityAttributes->setSignatureStatus(rhs.mSecurityAttributes->getSignatureStatus());
         mSecurityAttributes->setIdentity(rhs.mSecurityAttributes->getIdentity());
         mSecurityAttributes->setIdentityStrength(rhs.mSecurityAttributes->getIdentityStrength());
         mSecurityAttributes->setSigner(rhs.mSecurityAttributes->getSigner());
         mSecurityAttributes->setOutgoingEncryptionLevel(rhs.mSecurityAttributes->getOutgoingEncryptionLevel());
         mSecurityAttributes->setEncryptionPerformed(rhs.mSecurityAttributes->encryptionPerformed());
      }
      else
      {
         if (mSecurityAttributes.get())
         {
            mSecurityAttributes.reset();
         }
      }
      mOutboundDecorators = rhs.mOutboundDecorators;
   }

   return *this;
}

SipMessage::~SipMessage()
{
   cleanUp();
}

void
SipMessage::cleanUp()
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      delete mHeaders[i];
      mHeaders[i] = 0;
   }

   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      delete i->second;
   }
   mUnknownHeaders.clear();
   
   for (vector<char*>::iterator i = mBufferList.begin();
        i != mBufferList.end(); i++)
   {
      delete [] *i;
   }
   mBufferList.clear();

   delete mStartLine;
   mStartLine = 0;
   delete mContents;
   mContents = 0;
   delete mContentsHfv;
   mContentsHfv = 0;
   delete mForceTarget;
   mForceTarget = 0;
}

SipMessage*
SipMessage::make(const Data& data,  bool isExternal)
{
   Transport* external = (Transport*)(0xFFFF);
   SipMessage* msg = new SipMessage(isExternal ? external : 0);

   size_t len = data.size();
   char *buffer = new char[len + 5];

   msg->addBuffer(buffer);
   memcpy(buffer,data.data(), len);
   MsgHeaderScanner msgHeaderScanner;
   msgHeaderScanner.prepareForMessage(msg);
   
   char *unprocessedCharPtr;
   if (msgHeaderScanner.scanChunk(buffer, len, &unprocessedCharPtr) != MsgHeaderScanner::scrEnd)
   {
      DebugLog(<<"Scanner rejecting buffer as unparsable / fragmented.");
      DebugLog(<< data);
      delete msg; 
      msg = 0; 
      return 0;
   }

   // no pp error
   unsigned int used = unprocessedCharPtr - buffer;

   if (used < len)
   {
      // body is present .. add it up.
      // NB. The Sip Message uses an overlay (again)
      // for the body. It ALSO expects that the body
      // will be contiguous (of course).
      // it doesn't need a new buffer in UDP b/c there
      // will only be one datagram per buffer. (1:1 strict)

      msg->setBody(buffer+used,len-used);
      //DebugLog(<<"added " << len-used << " byte body");
   }

   return msg;
}

void
SipMessage::parseAllHeaders()
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      ParserContainerBase* pc=0;
      if(mHeaders[i])
      {
         ensureHeaders((Headers::Type)i,!Headers::isMulti((Headers::Type)i));
         if(!(pc=mHeaders[i]->getParserContainer()))
         {
            pc = HeaderBase::getInstance((Headers::Type)i)->makeContainer(mHeaders[i]);
            mHeaders[i]->setParserContainer(pc);
         }
      
         pc->parseAll();
      }
   }

   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      ParserContainerBase* scs=0;
      if(!(scs=i->second->getParserContainer()))
      {
         scs=new ParserContainer<StringCategory>(i->second,Headers::RESIP_DO_NOT_USE);
         i->second->setParserContainer(scs);
      }
      
      scs->parseAll();
   }
   
   assert(mStartLine);
   ParserContainerBase* slc = 0;

   if(!(slc=mStartLine->getParserContainer()))
   {
      if(mRequest)
      {
         slc=new ParserContainer<RequestLine>(mStartLine,Headers::NONE);
      }
      else if(mResponse)
      {
         slc=new ParserContainer<StatusLine>(mStartLine,Headers::NONE);
      }
      else
      {
         assert(0);
      }
      mStartLine->setParserContainer(slc);
   }

   slc->parseAll();
   
   getContents();
}

const Data& 
SipMessage::getTransactionId() const
{
   if (!this->exists(h_Vias) || this->header(h_Vias).empty())
   {
      InfoLog (<< "Bad message with no Vias: " << *this);
      throw Exception("No Via in message", __FILE__,__LINE__);
   }
   
   assert(exists(h_Vias) && !header(h_Vias).empty());
   if( exists(h_Vias) && header(h_Vias).front().exists(p_branch) 
       && header(h_Vias).front().param(p_branch).hasMagicCookie() 
       && (!header(h_Vias).front().param(p_branch).getTransactionId().empty())
     )
   {
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

//#define VONAGE_FIX
#ifndef VONAGE_FIX         
      strm << header(h_RequestLine).uri().scheme();
      strm << header(h_RequestLine).uri().user();
      strm << header(h_RequestLine).uri().host();
      strm << header(h_RequestLine).uri().port();
      strm << header(h_RequestLine).uri().password();
      strm << header(h_RequestLine).uri().commutativeParameterHash();
#endif
      if (exists(h_Vias) && !header(h_Vias).empty())
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

      strm << header(h_CallID).value();

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
   else
   {
      InfoLog (<< "Trying to compute a transaction id on a 2543 response. Drop the response");
      DebugLog (<< *this);
      throw Exception("Drop invalid 2543 response", __FILE__, __LINE__);
   }
}

const Data&
SipMessage::getRFC2543TransactionId() const
{
   if(!( exists(h_Vias) && !header(h_Vias).empty() && 
         header(h_Vias).front().exists(p_branch) &&
         header(h_Vias).front().param(p_branch).hasMagicCookie() ) )
   {
      if (mRFC2543TransactionId.empty())
      {
         compute2543TransactionHash();
      }
   }
   return mRFC2543TransactionId;
}


Data
SipMessage::getCanonicalIdentityString() const
{
   Data result;
   DataStream strm(result);
   
   // digest-string = addr-spec ":" addr-spec ":" callid ":" 1*DIGIT SP method ":"
   //             SIP-Date ":" [ addr-spec ] ":" message-body
  
   strm << header(h_From).uri();
   strm << Symbols::BAR;
   
   strm << header(h_To).uri();
   strm << Symbols::BAR;
   
   strm << header(h_CallId).value();
   strm << Symbols::BAR;
   
   header(h_CSeq).sequence(); // force parsed
   header(h_CSeq).encodeParsed( strm );
   strm << Symbols::BAR;
   
   // if there is no date, it will throw 
   if ( !exists(h_Date) )
   {
      WarningLog( << "Computing Identity on message with no Date header" );
      // TODO FIX - should it have a throw here ???? Help ???
   }
   header(h_Date).dayOfMonth(); // force it to be parsed 
   header(h_Date).encodeParsed( strm );
   strm << Symbols::BAR;
   
   if ( exists(h_Contacts) )
   { 
      if ( header(h_Contacts).front().isAllContacts() )
      {
         strm << Symbols::STAR;
      }
      else
      {
         strm << header(h_Contacts).front().uri();
      }
   }
   strm << Symbols::BAR;
   
   // bodies 
   if (mContents != 0)
   {
      mContents->encode(strm);
   }
   else if (mContentsHfv != 0)
   {
      mContentsHfv->encode(strm);
   }

   strm.flush();

   DebugLog( << "Indentity Canonical String is: " << result );
   
   return result;
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

resip::MethodTypes
SipMessage::method() const
{
   resip::MethodTypes res=UNKNOWN;
   try
   {
      if(isRequest())
      {
         res=header(h_RequestLine).getMethod();
      }
      else if(isResponse())
      {
         res=header(h_CSeq).method();
      }
      else
      {
         assert(0);
      }
   }
   catch(resip::ParseException&)
   {
   }
   
   return res;
}

std::ostream&
SipMessage::encodeBrief(std::ostream& str) const
{
   static const Data  request("SipReq:  ");
   static const Data response("SipResp: ");
   static const Data tid(" tid=");
   static const Data contact(" contact=");
   static const Data cseq(" cseq=");
   static const Data slash(" / ");
   static const Data wire(" from(wire)");
   static const Data ftu(" from(tu)");
   static const Data tlsd(" tlsd=");

   if (isRequest()) 
   {
      str << request;
      MethodTypes meth = header(h_RequestLine).getMethod();
      if (meth != UNKNOWN)
      {
         str << getMethodName(meth);
      }
      else
      {
         str << header(h_RequestLine).unknownMethodName();
      }
      
      str << Symbols::SPACE;
      str << header(h_RequestLine).uri().getAor();
   }
   else if (isResponse())
   {
      str << response;
      str << header(h_StatusLine).responseCode();
   }
   if (exists(h_Vias) && !this->header(h_Vias).empty())
   {
      str << tid;
      try
      {
         str << getTransactionId();
      }
      catch(SipMessage::Exception&)
      {
         str << "BAD-VIA";
      }
   }
   else
   {
      str << " NO-VIAS ";
   }

   str << cseq;
   if (header(h_CSeq).method() != UNKNOWN)
   {
      str << getMethodName(header(h_CSeq).method());
   }
   else
   {
      str << header(h_CSeq).unknownMethodName();
   }

   try
   {
      if (exists(h_Contacts) && !header(h_Contacts).empty())
      {
         str << contact;
         str << header(h_Contacts).front().uri().getAor();
      }
   }
   catch(resip::ParseException&)
   {
      str << " MALFORMED CONTACT ";
   }
   
   str << slash;
   str << header(h_CSeq).sequence();
   str << (mIsExternal ? wire : ftu);
   if (!mTlsDomain.empty())
   {
      str << tlsd << mTlsDomain;
   }
   
   return str;
}

bool
SipMessage::isClientTransaction() const
{
   assert(mRequest || mResponse);
   return ((mIsExternal && mResponse) || (!mIsExternal && mRequest));
}

std::ostream& 
SipMessage::encode(std::ostream& str) const
{
   return encode(str, false);
}

std::ostream& 
SipMessage::encodeSipFrag(std::ostream& str) const
{
   return encode(str, true);
}

// dynamic_cast &str to DataStream* to avoid CountStream?

std::ostream& 
SipMessage::encode(std::ostream& str, bool isSipFrag) const
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
            mHeaders[i]->encode(i, str);
         }
      }
      else
      {
         if (mContents != 0)
         {
            size_t size;
            {
               CountStream cs(size);
               mContents->encode(cs);
            }
            str << "Content-Length: " << size << "\r\n";
         }
         else if (mContentsHfv != 0)
         {
            str << "Content-Length: " << mContentsHfv->mFieldLength << "\r\n";
         }
         else if (!isSipFrag)
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
SipMessage::setBody(const char* start, UInt32 len)
{
   if(checkContentLength)
   {
      if(exists(h_ContentLength))
      {
         try
         {
            header(h_ContentLength).checkParsed();
         }
         catch(resip::ParseException& e)
         {
            if(mInvalid)
            {
               mReason+=",";
            }

            mInvalid=true; 
            mReason+="Malformed Content-Length";
            InfoLog(<< "Malformed Content-Length. Ignoring. " << e);
            header(h_ContentLength).value()=len;
         }
         
         UInt32 contentLength=header(h_ContentLength).value();
         
         if(len > contentLength)
         {
            InfoLog(<< (len-contentLength) << " extra bytes after body. Ignoring these bytes.");
         }
         else if(len < contentLength)
         {
            InfoLog(<< "Content Length is "<< (contentLength-len) << " bytes larger than body!"
                     << " (We are supposed to 400 this) ");

            if(mInvalid)
            {
               mReason+=",";
            }

            mInvalid=true; 
            mReason+="Bad Content-Length (larger than datagram)";
            header(h_ContentLength).value()=len;
            contentLength=len;
                     
         }
         
         mContentsHfv = new HeaderFieldValue(start,contentLength);
      }
      else
      {
         InfoLog(<< "Message has a body, but no Content-Length header.");
         mContentsHfv = new HeaderFieldValue(start,len);
      }
   }
   else
   {
      mContentsHfv = new HeaderFieldValue(start,len);
   }
}

void
SipMessage::setContents(auto_ptr<Contents> contents)
{
   Contents* contentsP = contents.release();

   delete mContents;
   mContents = 0;
   delete mContentsHfv;
   mContentsHfv = 0;

   if (contentsP == 0)
   {
      // The semantics of setContents(0) are to delete message contents
      remove(h_ContentType);
      remove(h_ContentDisposition);
      remove(h_ContentTransferEncoding);
      remove(h_ContentLanguages);
      return;
   }

   mContents = contentsP;

   // copy contents headers into message
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

void 
SipMessage::setContents(const Contents* contents)
{ 
   if (contents)
   {
      setContents(auto_ptr<Contents>(contents->clone()));
   }
   else
   {
      setContents(auto_ptr<Contents>(0));
   }
}

Contents*
SipMessage::getContents() const
{
   if (mContents == 0 && mContentsHfv != 0)
   {
      if (!exists(h_ContentType))
      {
         StackLog(<< "SipMessage::getContents: ContentType header does not exist - implies no contents");
         return 0;
      }
      DebugLog(<< "SipMessage::getContents: " 
               << header(h_ContentType).type()
               << "/"
               << header(h_ContentType).subType());

      if ( ContentsFactoryBase::getFactoryMap().find(header(h_ContentType)) == ContentsFactoryBase::getFactoryMap().end() )
      {
         InfoLog(<< "SipMessage::getContents: got content type ("
                 << header(h_ContentType).type()
                 << "/"
                 << header(h_ContentType).subType()
                 << ") that is not known, "
                 << "returning as opaque application/octet-stream");
         mContents = ContentsFactoryBase::getFactoryMap()[OctetContents::getStaticType()]->create(mContentsHfv, OctetContents::getStaticType());
      }
      else
      {
         mContents = ContentsFactoryBase::getFactoryMap()[header(h_ContentType)]->create(mContentsHfv, header(h_ContentType));
      }
      assert( mContents );
      
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

auto_ptr<Contents>
SipMessage::releaseContents()
{
   // .bwc. auto_ptr owns the Contents. No other references allowed!
   auto_ptr<Contents> ret(getContents());
   mContents = 0;

   if (ret.get() != 0 && !ret->isWellFormed())
   {
      ret.reset(0);
   }

   // .bwc. At this point, the Contents object has been parsed, so we don't need
   // this anymore.
   delete mContentsHfv;
   mContentsHfv=0;

   return ret;
}

// unknown header interface
const StringCategories& 
SipMessage::header(const ExtensionHeader& headerName) const
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
            hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }
   // missing extension header
   assert(false);

   return *(StringCategories*)0;
}

StringCategories& 
SipMessage::header(const ExtensionHeader& headerName)
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
            hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }

   // create the list empty
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
   mUnknownHeaders.push_back(make_pair(headerName.getName(), hfvs));
   return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
}

bool
SipMessage::exists(const ExtensionHeader& symbol) const
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
SipMessage::remove(const ExtensionHeader& headerName)
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
         if(mHeaders[header]->size()==1 && !(Headers::isMulti(header)))
         {
            if(mInvalid)
            {
               mReason+=",";
            }
            mInvalid=true;
            mReason+="Multiple values in single-value header ";
            mReason += Headers::getHeaderName(header);
            return;
         }
         mHeaders[header]->push_back(new HeaderFieldValue(start, len));
      }
   }
   else
   {
      assert(headerLen >= 0);
      for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
           i != mUnknownHeaders.end(); i++)
      {
         if (i->first.size() == (unsigned int)headerLen &&
             strncasecmp(i->first.data(), headerName, headerLen) == 0)
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

Data&
SipMessage::getCompartmentId() 
{
   return mCompartmentId;
}

RequestLine& 
SipMessage::header(const RequestLineType& l)
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

const RequestLine& 
SipMessage::header(const RequestLineType& l) const
{
   assert (!isResponse());
   if (mStartLine == 0 )
   { 
      // request line missing
      assert(false);
   }
   return dynamic_cast<ParserContainer<RequestLine>*>(mStartLine->getParserContainer())->front();
}

StatusLine& 
SipMessage::header(const StatusLineType& l)
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

const StatusLine& 
SipMessage::header(const StatusLineType& l) const
{
   assert (!isRequest());
   if (mStartLine == 0 )
   { 
      // status line missing
      assert(false);
   }
   return dynamic_cast<ParserContainer<StatusLine>*>(mStartLine->getParserContainer())->front();
}

HeaderFieldValueList* 
SipMessage::ensureHeaders(Headers::Type type, bool single)
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
      if (hfvs->parsedEmpty())
      {
         // create an unparsed shared header field value // !dlb! when will this happen?
         hfvs->push_back(new HeaderFieldValue(Data::Empty.data(), 0));
      }
   }

   return hfvs;
}

HeaderFieldValueList* 
SipMessage::ensureHeaders(Headers::Type type, bool single) const
{
   HeaderFieldValueList* hfvs = mHeaders[type];
   
   // empty?
   if (hfvs == 0)
   {
      // header missing
      // assert(false);
      InfoLog( << "Missing Header [" << Headers::getHeaderName(type) << "]");      
      DebugLog (<< *this);
      throw Exception("Missing header " + Headers::getHeaderName(type), __FILE__, __LINE__);
   }
   // !dlb! not thrilled about checking this every access
   else if (single)
   {
      if (hfvs->parsedEmpty())
      {
         // !dlb! when will this happen?
         // assert(false);
         InfoLog( << "Missing Header " << Headers::getHeaderName(type) );
         DebugLog (<< *this);
         throw Exception("Empty header", __FILE__, __LINE__);
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

bool
SipMessage::empty(const HeaderBase& headerType) const
{
   return !mHeaders[headerType.getTypeNum()] || mHeaders[headerType.getTypeNum()]->parsedEmpty();
}

void
SipMessage::remove(const HeaderBase& headerType)
{
   delete mHeaders[headerType.getTypeNum()]; 
   mHeaders[headerType.getTypeNum()] = 0; 
};

#ifndef PARTIAL_TEMPLATE_SPECIALIZATION

#undef defineHeader
#define defineHeader(_header, _name, _type, _rfc)                                                       \
const H_##_header::Type&                                                                                \
SipMessage::header(const H_##_header& headerType) const                                                 \
{                                                                                                       \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);                           \
   if (hfvs->getParserContainer() == 0)                                                                 \
   {                                                                                                    \
      hfvs->setParserContainer(new ParserContainer<H_##_header::Type>(hfvs, headerType.getTypeNum()));  \
   }                                                                                                    \
   return dynamic_cast<ParserContainer<H_##_header::Type>*>(hfvs->getParserContainer())->front();       \
}                                                                                                       \
                                                                                                        \
H_##_header::Type&                                                                                      \
SipMessage::header(const H_##_header& headerType)                                                       \
{                                                                                                       \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), true);                           \
   if (hfvs->getParserContainer() == 0)                                                                 \
   {                                                                                                    \
      hfvs->setParserContainer(new ParserContainer<H_##_header::Type>(hfvs, headerType.getTypeNum()));  \
   }                                                                                                    \
   return dynamic_cast<ParserContainer<H_##_header::Type>*>(hfvs->getParserContainer())->front();       \
}

#undef defineMultiHeader
#define defineMultiHeader(_header, _name, _type, _rfc)                                          \
const H_##_header##s::Type&                                                                     \
SipMessage::header(const H_##_header##s& headerType) const                                      \
{                                                                                               \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);                  \
   if (hfvs->getParserContainer() == 0)                                                         \
   {                                                                                            \
      hfvs->setParserContainer(new H_##_header##s::Type(hfvs, headerType.getTypeNum()));        \
   }                                                                                            \
   return *dynamic_cast<H_##_header##s::Type*>(hfvs->getParserContainer());                     \
}                                                                                               \
                                                                                                \
H_##_header##s::Type&                                                                           \
SipMessage::header(const H_##_header##s& headerType)                                            \
{                                                                                               \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), false);                  \
   if (hfvs->getParserContainer() == 0)                                                         \
   {                                                                                            \
      hfvs->setParserContainer(new H_##_header##s::Type(hfvs, headerType.getTypeNum()));        \
   }                                                                                            \
   return *dynamic_cast<H_##_header##s::Type*>(hfvs->getParserContainer());                     \
}

defineHeader(ContentDisposition, "Content-Disposition", Token, "RFC 3261");
defineHeader(ContentEncoding, "Content-Encoding", Token, "RFC 3261");
defineHeader(MIMEVersion, "Mime-Version", Token, "RFC 3261");
defineHeader(Priority, "Priority", Token, "RFC 3261");
defineHeader(Event, "Event", Token, "RFC 3265");
defineHeader(SubscriptionState, "Subscription-State", Token, "RFC 3265");
defineHeader(SIPETag, "SIP-ETag", Token, "RFC 3903");
defineHeader(SIPIfMatch, "SIP-If-Match", Token, "RFC 3903");
defineHeader(ContentId, "Content-ID", Token, "RFC 2045");
defineMultiHeader(AllowEvents, "Allow-Events", Token, "RFC 3265");
defineHeader(Identity, "Identity", StringCategory, "draft-sip-identity-03");
defineMultiHeader(AcceptEncoding, "Accept-Encoding", Token, "RFC 3261");
defineMultiHeader(AcceptLanguage, "Accept-Language", Token, "RFC 3261");
defineMultiHeader(Allow, "Allow", Token, "RFC 3261");
defineMultiHeader(ContentLanguage, "Content-Language", Token, "RFC 3261");
defineMultiHeader(ProxyRequire, "Proxy-Require", Token, "RFC 3261");
defineMultiHeader(Require, "Require", Token, "RFC 3261");
defineMultiHeader(Supported, "Supported", Token, "RFC 3261");
defineMultiHeader(Unsupported, "Unsupported", Token, "RFC 3261");
defineMultiHeader(SecurityClient, "Security-Client", Token, "RFC 3329");
defineMultiHeader(SecurityServer, "Security-Server", Token, "RFC 3329");
defineMultiHeader(SecurityVerify, "Security-Verify", Token, "RFC 3329");
defineMultiHeader(RequestDisposition, "Request-Disposition", Token, "RFC 3841");
defineMultiHeader(Reason, "Reason", Token, "RFC 3326");
defineMultiHeader(Privacy, "Privacy", Token, "RFC 3323");
defineMultiHeader(PMediaAuthorization, "P-Media-Authorization", Token, "RFC 3313");
defineHeader(ReferSub, "Refer-Sub", Token, "draft-ietf-sip-refer-with-norefersub-03");
defineHeader(AnswerMode, "Answer-Mode", Token, "draft-ietf-answermode-01");
defineHeader(PrivAnswerMode, "Priv-Answer-Mode", Token, "draft-ietf-answermode-01");

defineMultiHeader(Accept, "Accept", Mime, "RFC 3261");
defineHeader(ContentType, "Content-Type", Mime, "RFC 3261");

defineMultiHeader(CallInfo, "Call-Info", GenericUri, "RFC 3261");
defineMultiHeader(AlertInfo, "Alert-Info", GenericUri, "RFC 3261");
defineMultiHeader(ErrorInfo, "Error-Info", GenericUri, "RFC 3261");
defineHeader(IdentityInfo, "Identity-Info", GenericUri, "draft-sip-identity-03");

defineMultiHeader(RecordRoute, "Record-Route", NameAddr, "RFC 3261");
defineMultiHeader(Route, "Route", NameAddr, "RFC 3261");
defineMultiHeader(Contact, "Contact", NameAddr, "RFC 3261");
defineHeader(From, "From", NameAddr, "RFC 3261");
defineHeader(To, "To", NameAddr, "RFC 3261");
defineHeader(ReplyTo, "Reply-To", NameAddr, "RFC 3261");
defineHeader(ReferTo, "Refer-To", NameAddr, "RFC 3515");
defineHeader(ReferredBy, "Referred-By", NameAddr, "RFC 3892");
defineMultiHeader(Path, "Path", NameAddr, "RFC 3327");
defineMultiHeader(AcceptContact, "Accept-Contact", NameAddr, "RFC 3841");
defineMultiHeader(RejectContact, "Reject-Contact", NameAddr, "RFC 3841");
defineMultiHeader(PAssertedIdentity, "P-Asserted-Identity", NameAddr, "RFC 3325");
defineMultiHeader(PPreferredIdentity, "P-Preferred-Identity", NameAddr, "RFC 3325");
defineHeader(PCalledPartyId, "P-Called-Party-ID", NameAddr, "RFC 3455");
defineMultiHeader(PAssociatedUri, "P-Associated-URI", NameAddr, "RFC 3455");
defineMultiHeader(ServiceRoute, "Service-Route", NameAddr, "RFC 3608");

defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory, "RFC ?");
defineHeader(Organization, "Organization", StringCategory, "RFC 3261");
defineHeader(Server, "Server", StringCategory, "RFC 3261");
defineHeader(Subject, "Subject", StringCategory, "RFC 3261");
defineHeader(UserAgent, "User-Agent", StringCategory, "RFC 3261");
defineHeader(Timestamp, "Timestamp", StringCategory, "RFC 3261");

defineHeader(ContentLength, "Content-Length", UInt32Category, "RFC 3261");
defineHeader(MaxForwards, "Max-Forwards", UInt32Category, "RFC 3261");
defineHeader(MinExpires, "Min-Expires", Uint32Category, "RFC 3261");
defineHeader(RSeq, "RSeq", UInt32Category, "RFC 3261");

// !dlb! this one is not quite right -- can have (comment) after field value
defineHeader(RetryAfter, "Retry-After", UInt32Category, "RFC 3261");

defineHeader(Expires, "Expires", ExpiresCategory, "RFC 3261");
defineHeader(SessionExpires, "Session-Expires", ExpiresCategory, "RFC 4028");
defineHeader(MinSE, "Min-SE", ExpiresCategory, "RFC 4028");

defineHeader(CallID, "Call-ID", CallID, "RFC 3261");
defineHeader(Replaces, "Replaces", CallID, "RFC 3261");
defineHeader(InReplyTo, "In-Reply-To", CallID, "RFC 3261");
defineHeader(Join, "Join", CallId, "RFC 3911");
defineHeader(TargetDialog, "Target-Dialog", CallId, "Target Dialog draft");

defineHeader(AuthenticationInfo, "Authentication-Info", Auth, "RFC 3261");
defineMultiHeader(Authorization, "Authorization", Auth, "RFC 3261");
defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth, "RFC 3261");
defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth, "RFC 3261");
defineMultiHeader(WWWAuthenticate, "Www-Authenticate", Auth, "RFC 3261");

defineHeader(CSeq, "CSeq", CSeqCategory, "RFC 3261");
defineHeader(Date, "Date", DateCategory, "RFC 3261");
defineMultiHeader(Warning, "Warning", WarningCategory, "RFC 3261");
defineMultiHeader(Via, "Via", Via, "RFC 3261");
defineHeader(RAck, "RAck", RAckCategory, "RFC 3262");
defineHeader(RemotePartyId, "Remote-Party-ID", NameAddr, "draft-ietf-sip-privacy-04");

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
SipMessage::setForceTarget(const Uri& uri)
{
   if (mForceTarget)
   {
      *mForceTarget = uri;
   }
   else
   {
      mForceTarget = new Uri(uri);
   }
}

void
SipMessage::clearForceTarget()
{
   delete mForceTarget;
   mForceTarget = 0;
}

const Uri&
SipMessage::getForceTarget() const
{
   assert(mForceTarget);
   return *mForceTarget;
}

bool
SipMessage::hasForceTarget() const
{
   return (mForceTarget != 0);
}

SipMessage& 
SipMessage::mergeUri(const Uri& source)
{
   header(h_RequestLine).uri() = source;
   header(h_RequestLine).uri().removeEmbedded();

   if (source.exists(p_method))
   {
      header(h_RequestLine).method() = getMethodType(source.param(p_method));
      header(h_RequestLine).uri().remove(p_method);      
   }           
   
   //19.1.5
   //dangerous headers not included in merge:
   // From, Call-ID, Cseq, Via, Record Route, Route, Accept, Accept-Encoding,
   // Accept-Langauge, Allow, Contact, Organization, Supported, User-Agent

   //from the should-verify section, remove for now, some never seem to make
   //sense:  
   // Content-Encoding, Content-Language, Content-Length, Content-Type, Date,
   // Mime-Version, and TimeStamp

   if (source.hasEmbedded())
   {
      h_AuthenticationInfo.merge(*this, source.embedded());
      h_ContentTransferEncoding.merge(*this, source.embedded());
      h_Event.merge(*this, source.embedded());
      h_Expires.merge(*this, source.embedded());
      h_SessionExpires.merge(*this, source.embedded());
      h_MinSE.merge(*this, source.embedded());
      h_InReplyTo.merge(*this, source.embedded());
      h_MaxForwards.merge(*this, source.embedded());
      h_MinExpires.merge(*this, source.embedded());
      h_Priority.merge(*this, source.embedded());
      h_ReferTo.merge(*this, source.embedded());
      h_ReferredBy.merge(*this, source.embedded());
      h_Replaces.merge(*this, source.embedded());
      h_ReplyTo.merge(*this, source.embedded());
      h_RetryAfter.merge(*this, source.embedded());
      h_Server.merge(*this, source.embedded());
      h_SIPETag.merge(*this, source.embedded());
      h_SIPIfMatch.merge(*this, source.embedded());
      h_Subject.merge(*this, source.embedded());
      h_SubscriptionState.merge(*this, source.embedded());
      h_To.merge(*this, source.embedded());
      h_Warnings.merge(*this, source.embedded());

      h_SecurityClients.merge(*this, source.embedded());
      h_SecurityServers.merge(*this, source.embedded());
      h_SecurityVerifys.merge(*this, source.embedded());

      h_Authorizations.merge(*this, source.embedded());
      h_ProxyAuthenticates.merge(*this, source.embedded());
      h_WWWAuthenticates.merge(*this, source.embedded());
      h_ProxyAuthorizations.merge(*this, source.embedded());

      h_AlertInfos.merge(*this, source.embedded());
      h_AllowEvents.merge(*this, source.embedded());
      h_CallInfos.merge(*this, source.embedded());
      h_ErrorInfos.merge(*this, source.embedded());
      h_ProxyRequires.merge(*this, source.embedded());
      h_Requires.merge(*this, source.embedded());
      h_Unsupporteds.merge(*this, source.embedded());
      h_AnswerMode.merge(*this, source.embedded());
      h_PrivAnswerMode.merge(*this, source.embedded());

      h_RSeq.merge(*this, source.embedded());
      h_RAck.merge(*this, source.embedded());
   }   
   //unknown header merge
   return *this;   
}

void 
SipMessage::setSecurityAttributes(auto_ptr<SecurityAttributes> sec) const
{
   mSecurityAttributes = sec;
}

void
SipMessage::callOutboundDecorators(const Tuple &src, const Tuple &dest)
{
  std::vector<MessageDecorator*>::iterator i;
  for (i = mOutboundDecorators.begin();
       i != mOutboundDecorators.end(); i++)
  {
    (*i)->decorateMessage(*this, src, dest);
  }
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
