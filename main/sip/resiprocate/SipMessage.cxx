#include "sipstack/SipMessage.hxx"
#include "sipstack/HeaderFieldValueList.hxx"
#include "util/Logger.hxx"
#include "util/compat.hxx"
#include "util/vmd5.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

SipMessage::SipMessage(bool fromWire)
   : mIsExternal(fromWire),
     mHaveFixedDest(false),
     mFixedDest(),
     mStartLine(0),
     mBody(0),
     mRequest(false),
     mResponse(false)
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      mHeaders[i] = 0;
   }

}

SipMessage::SipMessage(const SipMessage& from)
   : mIsExternal(from.mIsExternal),
     mStartLine(0),
     mBody(0),
     mRequest(from.mRequest),
     mResponse(from.mResponse)
{
   if (this != &from)
   {
      if (from.hasFixedDest())
      {
         mHaveFixedDest = true;
         mFixedDest = from.getFixedDest();
      }
  
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
      if (from.mBody != 0)
      {
         mBody = new HeaderFieldValueList(*from.mBody);
      }
   }
}

SipMessage::~SipMessage()
{
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
   delete mBody;
}

const Data& 
SipMessage::getTransactionId() const
{
   assert (!header(h_Vias).empty());
   assert (header(h_Vias).front().exists(p_branch));
   assert (!header(h_Vias).front().param(p_branch).transactionId().empty());
   if( header(h_Vias).front().param(p_branch).hasMagicCookie() )
   {
       return header(h_Vias).front().param(p_branch).transactionId();
   }
   else
   {
       if( mRFC2543TransactionId.empty() )
       {
           Data key = header(h_CallId).value().lowercase() +
                      header(h_From).uri().param(p_tag).lowercase() +
                      Data( header(h_CSeq).sequence() ) +
                      header(h_Vias).front().param(p_branch).transactionId().lowercase();
           MD5Context context;
           MD5Init( &context );
           MD5Update( &context,
                      reinterpret_cast<unsigned const char*>(key.data()),
                      key.size() );
           md5byte digest[16];
           MD5Final( digest, &context );
           mRFC2543TransactionId = Data( reinterpret_cast<const char*>(digest), 16 );
       }
       return mRFC2543TransactionId;
   }
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
      result += "Request: ";
      result += header(h_RequestLine).uri().getAor();
   }
   else if (isResponse())
   {
      result += "Response: ";
      result += header(h_StatusLine).responseCode();
   }

   return result;
}

std::ostream& 
SipMessage::encode(std::ostream& str) const
{
   // !dlb! calculate content-length -- ask body
   if (mStartLine != 0)
   {
      mStartLine->encode(Headers::NONE, str);
   }
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (mHeaders[i] !=0)
      {
         mHeaders[i]->encode((Headers::Type)i, str);
      }
   }
   str << Symbols::CRLF;
   
   if (mBody != 0)
   {
      mBody->encode(Headers::NONE, str);
   }
   
   return str;
}

void
SipMessage::addBuffer(char* buf)
{
   mBufferList.push_back(buf);
}

void
SipMessage::setSource(const Transport::Tuple& addr)
{
   mSource = addr;
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
   mBody = new HeaderFieldValueList;
   mBody->push_back(new HeaderFieldValue(start, len));
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

bool
SipMessage::hasFixedDest() const
{
   return mHaveFixedDest;
}

Data
SipMessage::getFixedDest() const
{
   return mFixedDest;
}

void
SipMessage::setFixedDest(const Data& dest)
{
   mFixedDest = dest;
   mHaveFixedDest = true;
}

void
SipMessage::clearFixedDest()
{
   mFixedDest = "";
   mHaveFixedDest = false;
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

Authorization_Header::Type&
SipMessage::header(const Authorization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authorization_Header::Type>(hfvs,
                                                                               headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Authorization_Header::Type>*>(hfvs->getParserContainer())->front();
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

Proxy_Authenticate_Header::Type&
SipMessage::header(const Proxy_Authenticate_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authenticate_Header::Type>(hfvs,
                                                                                    headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Proxy_Authenticate_Header::Type>*>(hfvs->getParserContainer())->front();
};

Proxy_Authorization_Header::Type&
SipMessage::header(const Proxy_Authorization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authorization_Header::Type>(hfvs,
                                                                                     headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<Proxy_Authorization_Header::Type>*>(hfvs->getParserContainer())->front();
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

WWW_Authenticate_Header::Type&
SipMessage::header(const WWW_Authenticate_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<WWW_Authenticate_Header::Type>(hfvs,
                                                                                  headerType.getTypeNum()));
   }
   return dynamic_cast<ParserContainer<WWW_Authenticate_Header::Type>*>(hfvs->getParserContainer())->front();
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
