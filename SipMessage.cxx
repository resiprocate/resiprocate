#include <sipstack/SipMessage.hxx>
#include <sipstack/HeaderFieldValueList.hxx>

using namespace Vocal2;
using namespace std;

int strcasecmp(const char*, const char*);
int strncasecmp(const char*, const char*, int len);

SipMessage::SipMessage()
   : mIsExternal(true),
     mFixedDest(false),
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
// TODO - I have no idea if this should be true or false 
   : mIsExternal(false),
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
         delete mHeaders[i];
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
         mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(i->first,
                                                                     new HeaderFieldValueList(*i->second)));
      }
      if (from.mStartLine != 0)
      {
         mStartLine = new HeaderFieldValue(*from.mStartLine);
      }
      if (from.mBody != 0)
      {
         mBody = new HeaderFieldValue(*from.mBody);
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

   // no ParserContainer for startLine
   if (mStartLine != 0)
   {
      delete mStartLine->mParserCategory;
      delete mStartLine;
   }

   // no ParserContainer for body
   if (mBody != 0)
   {
      delete mBody->mParserCategory;
      delete mBody;
   }
}

const Data& 
SipMessage::getTransactionId() const
{
   // !jf! lookup the transactionId the first time and cache it
   return mTransactionId;
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
   assert(0);
   return Data();
}

std::ostream& 
SipMessage::encode(std::ostream& str) const
{
   // !dlb! calculate content-length -- ask body
   if (mStartLine != 0)
   {
      mStartLine->encode(str);
      str << Symbols::CRLF;
   }
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (mHeaders[i] !=0)
      {
         if (mHeaders[i]->getParserContainer() != 0)
         {
            mHeaders[i]->getParserContainer()->encode(str);
         }
         else
         {
            for (HeaderFieldValueList::const_iterator j = mHeaders[i]->begin();
                 j != mHeaders[i]->end(); j++)
            {
               str << Headers::HeaderNames[i] << Symbols::COLON << Symbols::SPACE;
               (*j)->encode(str);
               str << Symbols::CRLF;
            }
         }
      }
   }
   
   if (mBody != 0)
   {
      mBody->encode(str);
   }
   
   return str;
}

void
SipMessage::addBuffer(char* buf)
{
   mBufferList.push_back(buf);
}

void
SipMessage::setSource(const sockaddr_in& addr)
{
   mSource = addr;
}

void 
SipMessage::setStartLine(const char* st, int len)
{
   mStartLine = new HeaderFieldValue(st, len);
   ParseBuffer pb(mStartLine->mField, mStartLine->mFieldLength);
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
         StatusLine* parser = new StatusLine(mStartLine);
         mStartLine->mParserCategory = parser;
         //!dcm! should invoke the statusline parser here once it does limited validation
         mResponse = true;
      }
   }
   if (!mResponse)
   {
      RequestLine* parser = new RequestLine(mStartLine);
      mStartLine->mParserCategory = parser;
      //!dcm! should invoke the responseline parser here once it does limited validation
      mRequest = true;
   }
}

void 
SipMessage::setBody(const char* start, int len)
{
   mBody = new HeaderFieldValue(start, len);
}

// unknown header interface
// !dlb! need to convert existing header by enum to StringCategory for backward compatibility
StringCategories& 
SipMessage::header(const Data& headerName) const
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == headerName)
      {
         HeaderFieldValueList* hfvs = i->second;
         if (!hfvs->empty() && !hfvs->front()->isParsed())
         {
            for (HeaderFieldValueList::iterator j = hfvs->begin(); 
                 j != hfvs->end(); i++)
            {
               (*j)->mParserCategory = new StringCategory(*j);
            }
            
            hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs));
         }
         return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
      }
   }
   
   // create the list empty
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   hfvs->setParserContainer(new ParserContainer<StringCategory>(hfvs));
   mUnknownHeaders.push_back(make_pair(headerName, hfvs));
   return *dynamic_cast<ParserContainer<StringCategory>*>(hfvs->getParserContainer());
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
         mHeaders[header] = new HeaderFieldValueList();
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
         if (strncasecmp(i->first.c_str(), headerName, headerLen) == 0)
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

RequestLine& 
SipMessage::header(const RequestLineType& l) const
{
   if (isResponse())
   {
      throw Exception("Tried to retrieve a RequestLine from a Response", __FILE__, __LINE__);
   }
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
      RequestLine* parser = new RequestLine(mStartLine);
      mStartLine->mParserCategory = parser;
      mRequest = true;
   }
   return *dynamic_cast<RequestLine*>(mStartLine->mParserCategory);
}

StatusLine& 
SipMessage::header(const StatusLineType& l) const
{
   if (isRequest())
   {
      throw Exception("Tried to retrieve a StatusLine from a Request", __FILE__, __LINE__);
   }
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
      StatusLine* parser = new StatusLine(mStartLine);
      mStartLine->mParserCategory = parser;
      mResponse = true;
   }
   return *dynamic_cast<StatusLine*>(mStartLine->mParserCategory);
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
      hfvs->setParserContainer(new ParserContainer<CSeq_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<CSeq_Header::Type>*>(hfvs->getParserContainer())->front();
};

Call_ID_Header::Type&
SipMessage::header(const Call_ID_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Call_ID_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Call_ID_Header::Type>*>(hfvs->getParserContainer())->front();
};

Authentication_Info_Header::Type&
SipMessage::header(const Authentication_Info_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authentication_Info_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Authentication_Info_Header::Type>*>(hfvs->getParserContainer())->front();
};

Authorization_Header::Type&
SipMessage::header(const Authorization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Authorization_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Authorization_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Disposition_Header::Type&
SipMessage::header(const Content_Disposition_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Disposition_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Content_Disposition_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Encoding_Header::Type&
SipMessage::header(const Content_Encoding_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Encoding_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Content_Encoding_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Length_Header::Type&
SipMessage::header(const Content_Length_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Length_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Content_Length_Header::Type>*>(hfvs->getParserContainer())->front();
};

Content_Type_Header::Type&
SipMessage::header(const Content_Type_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Type_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Content_Type_Header::Type>*>(hfvs->getParserContainer())->front();
};

Date_Header::Type&
SipMessage::header(const Date_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Date_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Date_Header::Type>*>(hfvs->getParserContainer())->front();
};

Expires_Header::Type&
SipMessage::header(const Expires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Expires_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Expires_Header::Type>*>(hfvs->getParserContainer())->front();
};

From_Header::Type&
SipMessage::header(const From_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<From_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<From_Header::Type>*>(hfvs->getParserContainer())->front();
};

In_Reply_To_Header::Type&
SipMessage::header(const In_Reply_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<In_Reply_To_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<In_Reply_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

MIME_Version_Header::Type&
SipMessage::header(const MIME_Version_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<MIME_Version_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<MIME_Version_Header::Type>*>(hfvs->getParserContainer())->front();
};

Max_Forwards_Header::Type&
SipMessage::header(const Max_Forwards_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Max_Forwards_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Max_Forwards_Header::Type>*>(hfvs->getParserContainer())->front();
};

Min_Expires_Header::Type&
SipMessage::header(const Min_Expires_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Min_Expires_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Min_Expires_Header::Type>*>(hfvs->getParserContainer())->front();
};

Organization_Header::Type&
SipMessage::header(const Organization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Organization_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Organization_Header::Type>*>(hfvs->getParserContainer())->front();
};

Priority_Header::Type&
SipMessage::header(const Priority_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Priority_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Priority_Header::Type>*>(hfvs->getParserContainer())->front();
};

Proxy_Authenticate_Header::Type&
SipMessage::header(const Proxy_Authenticate_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authenticate_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Proxy_Authenticate_Header::Type>*>(hfvs->getParserContainer())->front();
};

Proxy_Authorization_Header::Type&
SipMessage::header(const Proxy_Authorization_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Authorization_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Proxy_Authorization_Header::Type>*>(hfvs->getParserContainer())->front();
};

Refer_To_Header::Type&
SipMessage::header(const Refer_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Refer_To_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Refer_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

Referred_By_Header::Type&
SipMessage::header(const Referred_By_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Referred_By_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Referred_By_Header::Type>*>(hfvs->getParserContainer())->front();
};

Replaces_Header::Type&
SipMessage::header(const Replaces_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Replaces_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Replaces_Header::Type>*>(hfvs->getParserContainer())->front();
};

Reply_To_Header::Type&
SipMessage::header(const Reply_To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Reply_To_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Reply_To_Header::Type>*>(hfvs->getParserContainer())->front();
};

Retry_After_Header::Type&
SipMessage::header(const Retry_After_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Retry_After_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Retry_After_Header::Type>*>(hfvs->getParserContainer())->front();
};

Server_Header::Type&
SipMessage::header(const Server_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Server_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Server_Header::Type>*>(hfvs->getParserContainer())->front();
};

Subject_Header::Type&
SipMessage::header(const Subject_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Subject_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Subject_Header::Type>*>(hfvs->getParserContainer())->front();
};

Timestamp_Header::Type&
SipMessage::header(const Timestamp_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Timestamp_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Timestamp_Header::Type>*>(hfvs->getParserContainer())->front();
};

To_Header::Type&
SipMessage::header(const To_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<To_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<To_Header::Type>*>(hfvs->getParserContainer())->front();
};

User_Agent_Header::Type&
SipMessage::header(const User_Agent_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<User_Agent_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<User_Agent_Header::Type>*>(hfvs->getParserContainer())->front();
};

WWW_Authenticate_Header::Type&
SipMessage::header(const WWW_Authenticate_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<WWW_Authenticate_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<WWW_Authenticate_Header::Type>*>(hfvs->getParserContainer())->front();
};

Warning_Header::Type&
SipMessage::header(const Warning_Header& headerType) const
{
   HeaderFieldValueList* hfvs = ensureHeader(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Warning_Header::Type>(hfvs));
   }
   return dynamic_cast<ParserContainer<Warning_Header::Type>*>(hfvs->getParserContainer())->front();
};

ParserContainer<Accept_MultiHeader::Type>&
SipMessage::header(const Accept_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Accept_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_Encoding_MultiHeader::Type>&
SipMessage::header(const Accept_Encoding_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_Encoding_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Accept_Encoding_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Accept_Language_MultiHeader::Type>&
SipMessage::header(const Accept_Language_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Accept_Language_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Accept_Language_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Alert_Info_MultiHeader::Type>&
SipMessage::header(const Alert_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Alert_Info_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Alert_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Allow_MultiHeader::Type>&
SipMessage::header(const Allow_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Allow_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Allow_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Call_Info_MultiHeader::Type>&
SipMessage::header(const Call_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Call_Info_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Call_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Contact_MultiHeader::Type>&
SipMessage::header(const Contact_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Contact_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Contact_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Content_Language_MultiHeader::Type>&
SipMessage::header(const Content_Language_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Content_Language_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Content_Language_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Error_Info_MultiHeader::Type>&
SipMessage::header(const Error_Info_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Error_Info_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Error_Info_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Proxy_Require_MultiHeader::Type>&
SipMessage::header(const Proxy_Require_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Proxy_Require_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Proxy_Require_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Record_Route_MultiHeader::Type>&
SipMessage::header(const Record_Route_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Record_Route_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Record_Route_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Require_MultiHeader::Type>&
SipMessage::header(const Require_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Require_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Require_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Route_MultiHeader::Type>&
SipMessage::header(const Route_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Route_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Route_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Subscription_State_MultiHeader::Type>&
SipMessage::header(const Subscription_State_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Subscription_State_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Subscription_State_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Supported_MultiHeader::Type>&
SipMessage::header(const Supported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Supported_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Supported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Unsupported_MultiHeader::Type>&
SipMessage::header(const Unsupported_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Unsupported_MultiHeader::Type>(hfvs));
   }
   return *dynamic_cast<ParserContainer<Unsupported_MultiHeader::Type>*>(hfvs->getParserContainer());
};

ParserContainer<Via_MultiHeader::Type>&
SipMessage::header(const Via_MultiHeader& headerType) const 
{
   HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum());
   if (hfvs->getParserContainer() == 0)
   {
      hfvs->setParserContainer(new ParserContainer<Via_MultiHeader::Type>(hfvs));
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
