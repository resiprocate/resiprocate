#include <sipstack/SipMessage.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/HeaderFieldValueList.hxx>

using namespace Vocal2;
using namespace std;

int strcasecmp(const char*, const char*);
int strncasecmp(const char*, const char*, int len);


#ifndef USE_METHOD_TEMPLATE
#include <sipstack/SipMessageExplicit.cxx>
#endif

SipMessage::SipMessage()
   : mIsExternal(true),
     mFixedDest(false),
     mStartLine(0)
{
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      mHeaders[i] = 0;
   }

   // pre-parse headers?
}

SipMessage::SipMessage(const SipMessage& from)
#ifdef WIN32
// TODO - I have no idea if this should be true or false 
: mIsExternal(false)
#else
#endif
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
         if (mHeaders[i] != 0)
         {
            mHeaders[i] = from.mHeaders[i]->clone();
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
                                                                     i->second->clone()));
      }
      if (from.mStartLine != 0)
      {
         mStartLine = from.mStartLine->clone();
      }
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
   //assert(0); // !jf!
   return true;
}

bool
SipMessage::isResponse() const
{
   return !isRequest();
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
   if (mStartLine != 0)
   {
      mStartLine->encode(str);
   }
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (mHeaders[i] !=0)
      {
         ParserContainerBase* parser = mHeaders[i]->getParserContainer();
         if (parser != 0)
         {
            parser->encode(str);
         }
         else
         {
            mHeaders[i]->encode(str);
         }
      }
   }

   if (mBody != 0)
   {
      mBody->encode(str);
   }
   
   return str;
}

SipMessage::~SipMessage()
{
   cleanUp();
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
SipMessage::setStartLine(char* start, int len)
{
   mStartLine = new HeaderFieldValue(start, len);
}

void 
SipMessage::setBody(char* start, int len)
{
   mBody = new HeaderFieldValue(start, len);
}

void
SipMessage::cleanUp()
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
   
   if (mStartLine != 0)
   {
      delete mStartLine;
   }
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
         if (i->second->first != 0 && !i->second->first->isParsed())
         {
            HeaderFieldValue* it = hfvs->first;
            while (it != 0)
            {
               it->mParserCategory = new StringCategory(it);
            }
            
            hfvs->setParserContainer(new StringCategories(hfvs));
         }
         return *dynamic_cast<StringCategories*>(hfvs->getParserContainer());
      }
   }
   
   // create the list empty
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   hfvs->setParserContainer(new StringCategories(hfvs));
   mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(headerName, hfvs));
   return *dynamic_cast<StringCategories*>(hfvs->getParserContainer());
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
      for(UnknownHeaders::iterator i = mUnknownHeaders.begin();
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
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
   }
   
   RequestLine* parser = new RequestLine(mStartLine);
   mStartLine->mParserCategory = parser;
   
   return *parser;
}


StatusLine& 
SipMessage::header(const StatusLineType& l) const
{
   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
   }
   
   StatusLine* parser = new StatusLine(mStartLine);
   mStartLine->mParserCategory = parser;
   
   return *parser;
}

HeaderFieldValueList* 
SipMessage::ensureHeader(int type) const
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
