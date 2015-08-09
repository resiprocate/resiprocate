#if defined(HAVE_CONFIG_H)
#include "config.h"
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
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

bool SipMessage::checkContentLength=true;

SipMessage::SipMessage(const Tuple *receivedTransportTuple)
   : mIsDecorated(false),
     mIsBadAck200(false),     
     mIsExternal(receivedTransportTuple != 0),  // may be modified later by setFromTU or setFromExternal
     mHeaders(StlPoolAllocator<HeaderFieldValueList*, PoolBase >(&mPool)),
#ifndef __SUNPRO_CC
     mUnknownHeaders(StlPoolAllocator<std::pair<Data, HeaderFieldValueList*>, PoolBase >(&mPool)),
#else
     mUnknownHeaders(),
#endif
     mRequest(false),
     mResponse(false),
     mInvalid(false),
     mCreatedTime(Timer::getTimeMicroSec()),
     mTlsDomain(Data::Empty)
{
   if(receivedTransportTuple)
   {
       mReceivedTransportTuple = *receivedTransportTuple;
   }
   // !bwc! TODO make this tunable
   mHeaders.reserve(16);
   clear();
}

SipMessage::SipMessage(const SipMessage& from)
   : mHeaders(StlPoolAllocator<HeaderFieldValueList*, PoolBase >(&mPool)),
#ifndef __SUNPRO_CC
     mUnknownHeaders(StlPoolAllocator<std::pair<Data, HeaderFieldValueList*>, PoolBase >(&mPool)),
#else
     mUnknownHeaders(),
#endif
     mCreatedTime(Timer::getTimeMicroSec())
{
   init(from);
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
      freeMem();
      init(rhs);
   }
   return *this;
}

SipMessage::~SipMessage()
{
//#define DINKYPOOL_PROFILING
#ifdef DINKYPOOL_PROFILING
   if (mPool.getHeapBytes() > 0)
   {
       InfoLog(<< "SipMessage mPool filled up and used " << mPool.getHeapBytes() << " bytes on the heap, consider increasing the mPool size (sizeof SipMessage is " << sizeof(SipMessage) << " bytes): msg="
           << std::endl << *this);
   }
   else
   {
       InfoLog(<< "SipMessage mPool used " << mPool.getPoolBytes() << " bytes of a total " << mPool.getPoolSizeBytes() << " bytes (sizeof SipMessage is " << sizeof(SipMessage) << " bytes): msg="
           << std::endl << *this);
   }
#endif
   freeMem();
}

void
SipMessage::clear(bool leaveResponseStuff)
{
   if(!leaveResponseStuff)
   {
      memset(mHeaderIndices,0,sizeof(mHeaderIndices));
      clearHeaders();
      
      // !bwc! The "invalid" 0 index.
      mHeaders.push_back(getEmptyHfvl());
      mBufferList.clear();
   }

   mUnknownHeaders.clear();

   mStartLine = 0;
   mContents = 0;
   mContentsHfv.clear();
   mForceTarget = 0;
   mReason=0;
   mOutboundDecorators.clear();
}

void
SipMessage::init(const SipMessage& rhs)
{
   clear();
   mIsDecorated = rhs.mIsDecorated;
   mIsBadAck200 = rhs.mIsBadAck200;
   mIsExternal = rhs.mIsExternal;
   mReceivedTransportTuple = rhs.mReceivedTransportTuple;
   mSource = rhs.mSource;
   mDestination = rhs.mDestination;
   mRFC2543TransactionId = rhs.mRFC2543TransactionId;
   mRequest = rhs.mRequest;
   mResponse = rhs.mResponse;
   mInvalid = rhs.mInvalid;
   if(!rhs.mReason)
   {
      mReason=0;
   }
   else
   {
      mReason = new Data(*rhs.mReason);
   }
   mTlsDomain = rhs.mTlsDomain;

   memcpy(&mHeaderIndices,&rhs.mHeaderIndices,sizeof(mHeaderIndices));

   // .bwc. Clear out the pesky invalid 0 index.
   clearHeaders();
   mHeaders.reserve(rhs.mHeaders.size());
   for (TypedHeaders::const_iterator i = rhs.mHeaders.begin();
        i != rhs.mHeaders.end(); i++)
   {
      mHeaders.push_back(getCopyHfvl(**i));
   }

   for (UnknownHeaders::const_iterator i = rhs.mUnknownHeaders.begin();
        i != rhs.mUnknownHeaders.end(); i++)
   {
      mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(
                                   i->first,
                                   getCopyHfvl(*i->second)));
   }
   if (rhs.mStartLine != 0)
   {
      mStartLine = rhs.mStartLine->clone(mStartLineMem);
   }
   if (rhs.mContents != 0)
   {
      mContents = rhs.mContents->clone();
   }
   else if (rhs.mContentsHfv.getBuffer() != 0)
   {
      mContentsHfv.copyWithPadding(rhs.mContentsHfv);
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

   for(std::vector<MessageDecorator*>::const_iterator i=rhs.mOutboundDecorators.begin(); i!=rhs.mOutboundDecorators.end();++i)
   {
      mOutboundDecorators.push_back((*i)->clone());
   }
}

void
SipMessage::freeMem(bool leaveResponseStuff)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      freeHfvl(i->second);
   }

   if(!leaveResponseStuff)
   {
      clearHeaders();

      for (vector<char*>::iterator i = mBufferList.begin();
           i != mBufferList.end(); i++)
      {
         delete [] *i;
      }
   }

   if(mStartLine)
   {
      mStartLine->~StartLine();
      mStartLine=0;
   }

   delete mContents;
   delete mForceTarget;
   delete mReason;

   for(std::vector<MessageDecorator*>::iterator i=mOutboundDecorators.begin();
         i!=mOutboundDecorators.end();++i)
   {
      delete *i;
   }
}

void
SipMessage::clearHeaders()
{
    for (TypedHeaders::iterator i = mHeaders.begin(); i != mHeaders.end(); i++)
    {
        freeHfvl(*i);
    }
    mHeaders.clear();
}

SipMessage*
SipMessage::make(const Data& data, bool isExternal)
{
   Tuple fakeWireTuple;
   fakeWireTuple.setType(UDP);
   SipMessage* msg = new SipMessage(isExternal ? &fakeWireTuple : 0);

   size_t len = data.size();
   char *buffer = new char[len + 5];

   msg->addBuffer(buffer);
   memcpy(buffer,data.data(), len);
   MsgHeaderScanner msgHeaderScanner;
   msgHeaderScanner.prepareForMessage(msg);
   
   char *unprocessedCharPtr;
   if (msgHeaderScanner.scanChunk(buffer, (unsigned int)len, &unprocessedCharPtr) != MsgHeaderScanner::scrEnd)
   {
      DebugLog(<<"Scanner rejecting buffer as unparsable / fragmented.");
      DebugLog(<< data);
      delete msg; 
      msg = 0; 
      return 0;
   }

   // no pp error
   unsigned int used = (unsigned int)(unprocessedCharPtr - buffer);

   if (used < len)
   {
      // body is present .. add it up.
      // NB. The Sip Message uses an overlay (again)
      // for the body. It ALSO expects that the body
      // will be contiguous (of course).
      // it doesn't need a new buffer in UDP b/c there
      // will only be one datagram per buffer. (1:1 strict)

      msg->setBody(buffer+used,UInt32(len-used));
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
      if(mHeaderIndices[i]>0)
      {
         HeaderFieldValueList* hfvl = ensureHeaders((Headers::Type)i);
         if(!Headers::isMulti((Headers::Type)i) && hfvl->parsedEmpty())
         {
            hfvl->push_back(0,0,false);
         }

         if(!(pc=hfvl->getParserContainer()))
         {
            pc = HeaderBase::getInstance((Headers::Type)i)->makeContainer(hfvl);
            hfvl->setParserContainer(pc);
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
         scs=makeParserContainer<StringCategory>(i->second,Headers::RESIP_DO_NOT_USE);
         i->second->setParserContainer(scs);
      }
      
      scs->parseAll();
   }
   
   resip_assert(mStartLine);

   mStartLine->checkParsed();
   
   getContents();
}

const Data& 
SipMessage::getTransactionId() const
{
   if (empty(h_Vias))
   {
      InfoLog (<< "Bad message with no Vias: " << *this);
      throw Exception("No Via in message", __FILE__,__LINE__);
   }
   
   resip_assert(exists(h_Vias) && !header(h_Vias).empty());
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
   resip_assert (mRFC2543TransactionId.empty());
   
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
      if (!empty(h_Vias))
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
   if(empty(h_Vias) ||
      !header(h_Vias).front().exists(p_branch) ||
      !header(h_Vias).front().param(p_branch).hasMagicCookie() ||
      header(h_Vias).front().param(p_branch).getTransactionId().empty())
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
   if ( empty(h_Date) )
   {
      WarningLog( << "Computing Identity on message with no Date header" );
      // TODO FIX - should it have a throw here ???? Help ???
   }
   header(h_Date).dayOfMonth(); // force it to be parsed 
   header(h_Date).encodeParsed( strm );
   strm << Symbols::BAR;
   
   if ( !empty(h_Contacts) )
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
   else if (mContentsHfv.getBuffer() != 0)
   {
      mContentsHfv.encode(strm);
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
         resip_assert(0);
      }
   }
   catch(resip::ParseException&)
   {
   }
   
   return res;
}

const Data&
SipMessage::methodStr() const
{
   if(method()!=UNKNOWN)
   {
      return getMethodName(method());
   }
   else
   {
      try
      {
         if(isRequest())
         {
            return header(h_RequestLine).unknownMethodName();
         }
         else if(isResponse())
         {
            return header(h_CSeq).unknownMethodName();
         }
         else
         {
            resip_assert(0);
         }
      }
      catch(resip::ParseException&)
      {
      }
   }
   return Data::Empty;
}

static const Data requestEB("SipReq:  ");
static const Data responseEB("SipResp: ");
static const Data tidEB(" tid=");
static const Data contactEB(" contact=");
static const Data cseqEB(" cseq=");
static const Data slashEB(" / ");
static const Data wireEB(" from(wire)");
static const Data ftuEB(" from(tu)");
static const Data tlsdEB(" tlsd=");
EncodeStream&
SipMessage::encodeBrief(EncodeStream& str) const
{
   if (isRequest()) 
   {
      str << requestEB;
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
      str << responseEB;
      str << header(h_StatusLine).responseCode();
   }
   if (!empty(h_Vias))
   {
      str << tidEB;
      try
      {
         str << getTransactionId();
      }
      catch(BaseException&)  // Could be SipMessage::Exception or ParseException
      {
         str << "BAD-VIA";
      }
   }
   else
   {
      str << " NO-VIAS ";
   }

   str << cseqEB;
   str << header(h_CSeq);

   try
   {
      if (!empty(h_Contacts))
      {
         str << contactEB;
         str << header(h_Contacts).front().uri().getAor();
      }
   }
   catch(resip::ParseException&)
   {
      str << " MALFORMED CONTACT ";
   }
   
   str << slashEB;
   str << header(h_CSeq).sequence();
   str << (mIsExternal ? wireEB : ftuEB);
   if (!mTlsDomain.empty())
   {
      str << tlsdEB << mTlsDomain;
   }
   
   return str;
}

bool
SipMessage::isClientTransaction() const
{
   resip_assert(mRequest || mResponse);
   return ((mIsExternal && mResponse) || (!mIsExternal && mRequest));
}

EncodeStream&
SipMessage::encode(EncodeStream& str) const
{
   return encode(str, false);
}

EncodeStream&
SipMessage::encodeSipFrag(EncodeStream& str) const
{
   return encode(str, true);
}

// dynamic_cast &str to DataStream* to avoid CountStream?

EncodeStream&
SipMessage::encode(EncodeStream& str, bool isSipFrag) const
{
   if (mStartLine != 0)
   {
      mStartLine->encode(str);
      str << "\r\n";
   }

   Data contents;
   if (mContents != 0)
   {
      oDataStream temp(contents);
      mContents->encode(temp);
   }
   else if (mContentsHfv.getBuffer() != 0)
   {
#if 0
      // !bwc! This causes an additional copy; sure would be nice to have a way
      // to get a data to take on a buffer with Data::Share _after_ construction
      contents.append(mContentsHfv.getBuffer(), mContentsHfv.getLength());
#else
      // .kw. Your wish is granted
      mContentsHfv.toShareData(contents);
#endif
   }

   for (UInt8 i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (i != Headers::ContentLength) // !dlb! hack...
      {
         if (mHeaderIndices[i] > 0)
         {
            mHeaders[mHeaderIndices[i]]->encode(i, str);
         }
      }
   }

   for (UnknownHeaders::const_iterator i = mUnknownHeaders.begin(); 
        i != mUnknownHeaders.end(); i++)
   {
      i->second->encode(i->first, str);
   }

   if(!isSipFrag || !contents.empty())
   {
      str << "Content-Length: " << contents.size() << "\r\n";
   }

   str << Symbols::CRLF;
   
   str << contents;
   return str;
}

EncodeStream&
SipMessage::encodeSingleHeader(Headers::Type type, EncodeStream& str) const
{
   if (mHeaderIndices[type] > 0)
   {
      mHeaders[mHeaderIndices[type]]->encode(type, str);
   }
   return str;
}

EncodeStream& 
SipMessage::encodeEmbedded(EncodeStream& str) const
{
   bool first = true;
   for (UInt8 i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (i != Headers::ContentLength)
      {
         if (mHeaderIndices[i] > 0)
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
            mHeaders[mHeaderIndices[i]]->encodeEmbedded(Headers::getHeaderName(i), str);
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

   if (mContents != 0 || mContentsHfv.getBuffer() != 0)
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
      Data contents;
      // !dlb! encode escaped for characters
      // .kw. what does that mean? what needs to be escaped?
      if(mContents != 0)
      {
         DataStream s(contents);
         mContents->encode(s);
      }
      else
      {
         // .kw. Early code did:
         // DataStream s(contents);
         // mContentsHfv->encode(str);
         // str << Embedded::encode(contents);
         // .kw. which I think is buggy b/c Hfv was written directly
         // to str and skipped the encode step via contents
         mContentsHfv.toShareData(contents);
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
   if(len >= 4 && !strncasecmp(st,"SIP/",4))
   {
      // Response
      mStartLine = new (mStartLineMem) StatusLine(st, len);
      //!dcm! should invoke the statusline parser here once it does limited validation
      mResponse = true;
   }
   else
   {
      // Request
      mStartLine = new (mStartLineMem) RequestLine(st, len);
      //!dcm! should invoke the responseline parser here once it does limited validation
      mRequest = true;
   }


// .bwc. This stuff is so needlessly complicated. Much, much simpler, faster,
// and more robust code above.
//   ParseBuffer pb(st, len);
//   const char* start;
//   start = pb.skipWhitespace();
//   pb.skipNonWhitespace();
//   MethodTypes method = getMethodType(start, pb.position() - start);
//   if (method == UNKNOWN) //probably a status line
//   {
//      start = pb.skipChar(Symbols::SPACE[0]);
//      pb.skipNonWhitespace();
//      if ((pb.position() - start) == 3)
//      {
//         mStartLine = new (mStartLineMem) StatusLine(st, len ,Headers::NONE);
//         //!dcm! should invoke the statusline parser here once it does limited validation
//         mResponse = true;
//      }
//   }
//   if (!mResponse)
//   {
//      mStartLine = new (mStartLineMem) RequestLine(st, len, Headers::NONE);
//      //!dcm! should invoke the responseline parser here once it does limited validation
//      mRequest = true;
//   }
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
            const_header(h_ContentLength).checkParsed();
         }
         catch(resip::ParseException& e)
         {
            if(!mReason)
            {
               mReason=new Data;
            }
            
            if(mInvalid)
            {
               mReason->append(",",1);
            }

            mInvalid=true; 
            mReason->append("Malformed Content-Length",24);
            InfoLog(<< "Malformed Content-Length. Ignoring. " << e);
            header(h_ContentLength).value()=len;
         }
         
         UInt32 contentLength=const_header(h_ContentLength).value();
         
         if(len > contentLength)
         {
            InfoLog(<< (len-contentLength) << " extra bytes after body. Ignoring these bytes.");
         }
         else if(len < contentLength)
         {
            InfoLog(<< "Content Length (" << contentLength << ") is "
                    << (contentLength-len) << " bytes larger than body (" << len << ")!"
                    << " (We are supposed to 400 this) ");

            if(!mReason)
            {
               mReason=new Data;
            }

            if(mInvalid)
            {
               mReason->append(",",1);
            }

            mInvalid=true; 
            mReason->append("Bad Content-Length (larger than datagram)",41);
            header(h_ContentLength).value()=len;
            contentLength=len;
                     
         }
         
         mContentsHfv.init(start,contentLength, false);
      }
      else
      {
         InfoLog(<< "Message has a body, but no Content-Length header.");
         mContentsHfv.init(start,len, false);
      }
   }
   else
   {
      mContentsHfv.init(start,len, false);
   }
}

void
SipMessage::setRawBody(const HeaderFieldValue& body)
{
   setContents(0);
   mContentsHfv = body;
}


void
SipMessage::setContents(auto_ptr<Contents> contents)
{
   Contents* contentsP = contents.release();

   delete mContents;
   mContents = 0;
   mContentsHfv.clear();

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
      resip_assert( header(h_ContentType).type() == mContents->getType().type() );
      resip_assert( header(h_ContentType).subType() == mContents->getType().subType() );
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
   if (mContents == 0 && mContentsHfv.getBuffer() != 0)
   {
      if (empty(h_ContentType) ||
            !const_header(h_ContentType).isWellFormed())
      {
         StackLog(<< "SipMessage::getContents: ContentType header does not exist - implies no contents");
         return 0;
      }
      DebugLog(<< "SipMessage::getContents: " 
               << const_header(h_ContentType).type()
               << "/"
               << const_header(h_ContentType).subType());

      if ( ContentsFactoryBase::getFactoryMap().find(const_header(h_ContentType)) == ContentsFactoryBase::getFactoryMap().end() )
      {
         InfoLog(<< "SipMessage::getContents: got content type ("
                 << const_header(h_ContentType).type()
                 << "/"
                 << const_header(h_ContentType).subType()
                 << ") that is not known, "
                 << "returning as opaque application/octet-stream");
         mContents = ContentsFactoryBase::getFactoryMap()[OctetContents::getStaticType()]->create(mContentsHfv, OctetContents::getStaticType());
      }
      else
      {
         mContents = ContentsFactoryBase::getFactoryMap()[const_header(h_ContentType)]->create(mContentsHfv, const_header(h_ContentType));
      }
      resip_assert( mContents );
      
      // copy contents headers into the contents
      if (!empty(h_ContentDisposition))
      {
         mContents->header(h_ContentDisposition) = const_header(h_ContentDisposition);
      }
      if (!empty(h_ContentTransferEncoding))
      {
         mContents->header(h_ContentTransferEncoding) = const_header(h_ContentTransferEncoding);
      }
      if (!empty(h_ContentLanguages))
      {
         mContents->header(h_ContentLanguages) = const_header(h_ContentLanguages);
      }
      if (!empty(h_ContentType))
      {
         mContents->header(h_ContentType) = const_header(h_ContentType);
      }
      // !dlb! Content-Transfer-Encoding?
   }
   return mContents;
}

auto_ptr<Contents>
SipMessage::releaseContents()
{
   Contents* c=getContents();
   // .bwc. auto_ptr owns the Contents. No other references allowed!
   auto_ptr<Contents> ret(c ? c->clone() : 0);
   setContents(std::auto_ptr<Contents>(0));

   if (ret.get() != 0 && !ret->isWellFormed())
   {
      ret.reset(0);
   }

   return ret;
}

// unknown header interface
const StringCategories& 
SipMessage::header(const ExtensionHeader& headerName) const
{
   for (UnknownHeaders::const_iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {      
      if (isEqualNoCase(i->first, headerName.getName()))
      {
         HeaderFieldValueList* hfvs = i->second;
         if (hfvs->getParserContainer() == 0)
         {
            SipMessage* nc_this(const_cast<SipMessage*>(this));
            hfvs->setParserContainer(nc_this->makeParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }
   // missing extension header
   resip_assert(false);

   return *(StringCategories*)0;
}

StringCategories& 
SipMessage::header(const ExtensionHeader& headerName)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (isEqualNoCase(i->first, headerName.getName()))
      {
         HeaderFieldValueList* hfvs = i->second;
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(makeParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }

   // create the list empty
   HeaderFieldValueList* hfvs = getEmptyHfvl();
   hfvs->setParserContainer(makeParserContainer<StringCategory>(hfvs, Headers::RESIP_DO_NOT_USE));
   mUnknownHeaders.push_back(make_pair(headerName.getName(), hfvs));
   return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
}

bool
SipMessage::exists(const ExtensionHeader& symbol) const
{
   for (UnknownHeaders::const_iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (isEqualNoCase(i->first, symbol.getName()))
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
      if (isEqualNoCase(i->first, headerName.getName()))
      {
         freeHfvl(i->second);
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
      resip_assert(header >= Headers::UNKNOWN && header < Headers::MAX_HEADERS);
      HeaderFieldValueList* hfvl=0;
      if (mHeaderIndices[header] == 0)
      {
         mHeaderIndices[header] = mHeaders.size();
         mHeaders.push_back(getEmptyHfvl());
         hfvl=mHeaders.back();
      }
      else
      {
         if(mHeaderIndices[header]<0)
         {
            // Adding to a previously removed header type; there is already an 
            // empty HeaderFieldValueList in mHeaders for this type, all we 
            // need to do is flip the sign to re-enable it.
            mHeaderIndices[header] *= -1;
         }
         hfvl=mHeaders[mHeaderIndices[header]];
      }

      if(Headers::isMulti(header))
      {
         if (len)
         {
            hfvl->push_back(start, len, false);
         }
      }
      else
      {
         if(hfvl->size()==1)
         {
            if(!mReason)
            {
               mReason=new Data;
            }
            
            if(mInvalid)
            {
               mReason->append(",",1);
            }
            mInvalid=true;
            mReason->append("Multiple values in single-value header ",39);
            (*mReason)+=Headers::getHeaderName(header);
            return;
         }
         hfvl->push_back(start ? start : Data::Empty.data(), len, false);
      }

   }
   else
   {
      resip_assert(headerLen >= 0);
      for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
           i != mUnknownHeaders.end(); i++)
      {
         if (i->first.size() == (unsigned int)headerLen &&
             strncasecmp(i->first.data(), headerName, headerLen) == 0)
         {
            // add to end of list
            if (len)
            {
               i->second->push_back(start, len, false);
            }
            return;
         }
      }

      // didn't find it, add an entry
      HeaderFieldValueList *hfvs = getEmptyHfvl();
      if (len)
      {
         hfvs->push_back(start, len, false);
      }
      mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(Data(headerName, headerLen),
                                                                  hfvs));
   }
}

RequestLine& 
SipMessage::header(const RequestLineType& l)
{
   resip_assert (!isResponse());
   if (mStartLine == 0 )
   { 
      mStartLine = new (mStartLineMem) RequestLine;
      mRequest = true;
   }
   return *static_cast<RequestLine*>(mStartLine);
}

const RequestLine& 
SipMessage::header(const RequestLineType& l) const
{
   resip_assert (!isResponse());
   if (mStartLine == 0 )
   { 
      // request line missing
      resip_assert(false);
   }
   return *static_cast<RequestLine*>(mStartLine);
}

StatusLine& 
SipMessage::header(const StatusLineType& l)
{
   resip_assert (!isRequest());
   if (mStartLine == 0 )
   { 
      mStartLine = new (mStartLineMem) StatusLine;
      mResponse = true;
   }
   return *static_cast<StatusLine*>(mStartLine);
}

const StatusLine& 
SipMessage::header(const StatusLineType& l) const
{
   resip_assert (!isRequest());
   if (mStartLine == 0 )
   { 
      // status line missing
      resip_assert(false);
   }
   return *static_cast<StatusLine*>(mStartLine);
}

HeaderFieldValueList* 
SipMessage::ensureHeaders(Headers::Type type)
{
   HeaderFieldValueList* hfvl=0;
   if(mHeaderIndices[type]!=0)
   {
      if(mHeaderIndices[type]<0)
      {
         // Accessing a previously removed header type; there is already an 
         // empty HeaderFieldValueList in mHeaders for this type, all we 
         // need to do is flip the sign to re-enable it.
         mHeaderIndices[type] *= -1;
      }
      hfvl = mHeaders[mHeaderIndices[type]];
   }
   else
   {
      // create the list with a new component
      mHeaders.push_back(getEmptyHfvl());
      hfvl=mHeaders.back();
      mHeaderIndices[type]=mHeaders.size()-1;
   }

   return hfvl;
}

HeaderFieldValueList* 
SipMessage::ensureHeader(Headers::Type type)
{
   HeaderFieldValueList* hfvl=0;
   if(mHeaderIndices[type]!=0)
   {
      if(mHeaderIndices[type]<0)
      {
         // Accessing a previously removed header type; there is already an 
         // empty HeaderFieldValueList in mHeaders for this type, all we 
         // need to do is flip the sign to re-enable it.
         mHeaderIndices[type] *= -1;
         hfvl = mHeaders[mHeaderIndices[type]];
         hfvl->push_back(0,0,false);
      }
      hfvl = mHeaders[mHeaderIndices[type]];
   }
   else
   {
      // create the list with a new component
      mHeaders.push_back(getEmptyHfvl());
      hfvl=mHeaders.back();
      mHeaderIndices[type]=mHeaders.size()-1;
      mHeaders.back()->push_back(0,0,false);
   }

   return hfvl;
}

void
SipMessage::throwHeaderMissing(Headers::Type type) const
{
   // header missing
   // assert(false);
   InfoLog( << "Missing Header [" << Headers::getHeaderName(type) << "]");      
   DebugLog (<< *this);
   throw Exception("Missing header " + Headers::getHeaderName(type), __FILE__, __LINE__);
}

// type safe header accessors
bool    
SipMessage::exists(const HeaderBase& headerType) const 
{
   return mHeaderIndices[headerType.getTypeNum()] > 0;
};

bool
SipMessage::empty(const HeaderBase& headerType) const
{
   return (mHeaderIndices[headerType.getTypeNum()] <= 0) || mHeaders[mHeaderIndices[headerType.getTypeNum()]]->parsedEmpty();
}

void
SipMessage::remove(Headers::Type type)
{
   if(mHeaderIndices[type] > 0)
   {
      // .bwc. The entry in mHeaders still remains after we do this; we retain 
      // our index (as a negative number, indicating that this header should 
      // not be encoded), in case this header type needs to be used later.
      mHeaders[mHeaderIndices[type]]->clear();
      mHeaderIndices[type] *= -1;
   }
};

#ifndef PARTIAL_TEMPLATE_SPECIALIZATION

#undef defineHeader
#define defineHeader(_header, _name, _type, _rfc)                                                       \
const H_##_header::Type&                                                                                \
SipMessage::header(const H_##_header& headerType) const                                                 \
{                                                                                                       \
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());                           \
   if (hfvs->getParserContainer() == 0)                                                                 \
   {                                                                                                    \
      SipMessage* nc_this(const_cast<SipMessage*>(this)); \
      hfvs->setParserContainer(nc_this->makeParserContainer<H_##_header::Type>(hfvs, headerType.getTypeNum()));  \
   }                                                                                                    \
   return static_cast<ParserContainer<H_##_header::Type>*>(hfvs->getParserContainer())->front();       \
}                                                                                                       \
                                                                                                        \
H_##_header::Type&                                                                                      \
SipMessage::header(const H_##_header& headerType)                                                       \
{                                                                                                       \
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());                           \
   if (hfvs->getParserContainer() == 0)                                                                 \
   {                                                                                                    \
      hfvs->setParserContainer(makeParserContainer<H_##_header::Type>(hfvs, headerType.getTypeNum()));  \
   }                                                                                                    \
   return static_cast<ParserContainer<H_##_header::Type>*>(hfvs->getParserContainer())->front();       \
}

#undef defineMultiHeader
#define defineMultiHeader(_header, _name, _type, _rfc)                                          \
const H_##_header##s::Type&                                                                     \
SipMessage::header(const H_##_header##s& headerType) const                                      \
{                                                                                               \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());                  \
   if (hfvs->getParserContainer() == 0)                                                         \
   {                                                                                            \
      SipMessage* nc_this(const_cast<SipMessage*>(this)); \
      hfvs->setParserContainer(nc_this->makeParserContainer<H_##_header##s::ContainedType>(hfvs, headerType.getTypeNum()));        \
   }                                                                                            \
   return *static_cast<H_##_header##s::Type*>(hfvs->getParserContainer());                     \
}                                                                                               \
                                                                                                \
H_##_header##s::Type&                                                                           \
SipMessage::header(const H_##_header##s& headerType)                                            \
{                                                                                               \
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());                  \
   if (hfvs->getParserContainer() == 0)                                                         \
   {                                                                                            \
      hfvs->setParserContainer(makeParserContainer<H_##_header##s::ContainedType>(hfvs, headerType.getTypeNum()));        \
   }                                                                                            \
   return *static_cast<H_##_header##s::Type*>(hfvs->getParserContainer());                     \
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
defineHeader(Identity, "Identity", StringCategory, "RFC 4474");
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
defineMultiHeader(Privacy, "Privacy", PrivacyCategory, "RFC 3323");
defineMultiHeader(PMediaAuthorization, "P-Media-Authorization", Token, "RFC 3313");
defineHeader(ReferSub, "Refer-Sub", Token, "RFC 4488");
defineHeader(AnswerMode, "Answer-Mode", Token, "draft-ietf-answermode-01");
defineHeader(PrivAnswerMode, "Priv-Answer-Mode", Token, "draft-ietf-answermode-01");

defineMultiHeader(Accept, "Accept", Mime, "RFC 3261");
defineHeader(ContentType, "Content-Type", Mime, "RFC 3261");

defineMultiHeader(CallInfo, "Call-Info", GenericUri, "RFC 3261");
defineMultiHeader(AlertInfo, "Alert-Info", GenericUri, "RFC 3261");
defineMultiHeader(ErrorInfo, "Error-Info", GenericUri, "RFC 3261");
defineHeader(IdentityInfo, "Identity-Info", GenericUri, "RFC 4474");

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
defineHeader(SecWebSocketKey, "Sec-WebSocket-Key", StringCategory, "RFC 6455");
defineHeader(SecWebSocketKey1, "Sec-WebSocket-Key1", StringCategory, "draft-hixie- thewebsocketprotocol-76");
defineHeader(SecWebSocketKey2, "Sec-WebSocket-Key2", StringCategory, "draft-hixie- thewebsocketprotocol-76");
defineHeader(Origin, "Origin", StringCategory, "draft-hixie- thewebsocketprotocol-76");
defineHeader(Host, "Host", StringCategory, "draft-hixie- thewebsocketprotocol-76");
defineHeader(SecWebSocketAccept, "Sec-WebSocket-Accept", StringCategory, "RFC 6455");
defineMultiHeader(Cookie, "Cookie", StringCategory, "RFC 6265");
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
defineHeader(FlowTimer, "Flow-Timer", UInt32Category, "RFC 5626");

defineHeader(Expires, "Expires", ExpiresCategory, "RFC 3261");
defineHeader(SessionExpires, "Session-Expires", ExpiresCategory, "RFC 4028");
defineHeader(MinSE, "Min-SE", ExpiresCategory, "RFC 4028");

defineHeader(CallID, "Call-ID", CallID, "RFC 3261");
defineHeader(Replaces, "Replaces", CallID, "RFC 3891");
defineHeader(InReplyTo, "In-Reply-To", CallID, "RFC 3261");
defineHeader(Join, "Join", CallId, "RFC 3911");
defineHeader(TargetDialog, "Target-Dialog", CallId, "RFC 4538");

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
defineMultiHeader(RemotePartyId, "Remote-Party-ID", NameAddr, "draft-ietf-sip-privacy-04"); // ?bwc? Not in 3323, should we keep?
defineMultiHeader(HistoryInfo, "History-Info", NameAddr, "RFC 4244");

defineHeader(PAccessNetworkInfo, "P-Access-Network-Info", Token, "RFC 3455");
defineHeader(PChargingVector, "P-Charging-Vector", Token, "RFC 3455");
defineHeader(PChargingFunctionAddresses, "P-Charging-Function-Addresses", Token, "RFC 3455");
defineMultiHeader(PVisitedNetworkID, "P-Visited-Network-ID", TokenOrQuotedStringCategory, "RFC 3455");

defineMultiHeader(UserToUser, "User-to-User", TokenOrQuotedStringCategory, "draft-ietf-cuss-sip-uui-17");

#endif

const HeaderFieldValueList*
SipMessage::getRawHeader(Headers::Type headerType) const
{
   if(mHeaderIndices[headerType]>0)
   {
      return mHeaders[mHeaderIndices[headerType]];
   }
   
   return 0;
}

void
SipMessage::setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType)
{
   HeaderFieldValueList* copy=0;
   if (mHeaderIndices[headerType] == 0)
   {
      mHeaderIndices[headerType]=mHeaders.size();
      copy=getCopyHfvl(*hfvs);
      mHeaders.push_back(copy);
   }
   else
   {
      if(mHeaderIndices[headerType]<0)
      {
         // Setting a previously removed header type; there is already an 
         // empty HeaderFieldValueList in mHeaders for this type, all we 
         // need to do is flip the sign to re-enable it.
         mHeaderIndices[headerType]=-mHeaderIndices[headerType];
      }
      copy = mHeaders[mHeaderIndices[headerType]];
      *copy=*hfvs;
   }
   if(!Headers::isMulti(headerType) && copy->parsedEmpty())
   {
      copy->push_back(0,0,false);
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
   resip_assert(mForceTarget);
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
SipMessage::setSecurityAttributes(auto_ptr<SecurityAttributes> sec)
{
   mSecurityAttributes = sec;
}

void
SipMessage::callOutboundDecorators(const Tuple &src, 
                                    const Tuple &dest,
                                    const Data& sigcompId)
{
   if(mIsDecorated)
   {
      rollbackOutboundDecorators();
   }

  std::vector<MessageDecorator*>::iterator i;
  for (i = mOutboundDecorators.begin();
       i != mOutboundDecorators.end(); i++)
  {
    (*i)->decorateMessage(*this, src, dest, sigcompId);
  }
  mIsDecorated = true;
}

void 
SipMessage::clearOutboundDecorators()
{
   while(!mOutboundDecorators.empty())
   {
      delete mOutboundDecorators.back();
      mOutboundDecorators.pop_back();
   }
}

void 
SipMessage::rollbackOutboundDecorators()
{
   std::vector<MessageDecorator*>::reverse_iterator r;
   for(r=mOutboundDecorators.rbegin(); r!=mOutboundDecorators.rend(); ++r)
   {
      (*r)->rollbackMessage(*this);
   }
   mIsDecorated = false;
}

void 
SipMessage::copyOutboundDecoratorsToStackCancel(SipMessage& cancel)
{
  std::vector<MessageDecorator*>::iterator i;
  for (i = mOutboundDecorators.begin();
       i != mOutboundDecorators.end(); i++)
  {
     if((*i)->copyToStackCancels())
     {
        cancel.addOutboundDecorator(std::auto_ptr<MessageDecorator>((*i)->clone()));
     }    
  }
}

void 
SipMessage::copyOutboundDecoratorsToStackFailureAck(SipMessage& ack)
{
  std::vector<MessageDecorator*>::iterator i;
  for (i = mOutboundDecorators.begin();
       i != mOutboundDecorators.end(); i++)
  {
     if((*i)->copyToStackFailureAcks())
     {
        ack.addOutboundDecorator(std::auto_ptr<MessageDecorator>((*i)->clone()));
     }    
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
 * vi: set shiftwidth=3 expandtab:
 */
