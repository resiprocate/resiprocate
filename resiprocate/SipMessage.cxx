#include <sipstack/SipMessage.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/HeaderFieldValueList.hxx>

using namespace Vocal2;
using namespace std;

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

SipMessage::SipMessage(const SipMessage& message)
{
   if (this != &message)
   {
      cleanUp();
      copyFrom(message);
   }
}

const Data& 
SipMessage::getTransactionId() const
{
   // !jf! lookup the transactionId the first time and cache it
   return mTransactionId;
}


SipMessage*
SipMessage::clone() const
{
   // no message buffer
   SipMessage* newMessage = new SipMessage();
   newMessage->copyFrom(*this);
   return newMessage;
}

bool
SipMessage::isRequest() const
{
   assert(0);
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
   return Data();
}


ostream& 
SipMessage::encode(ostream& str) const
{
   if (mStartLine != 0)
   {
      mStartLine->encode(str);
   }
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      if (mHeaders[i] !=0)
      {
         ParserCategory* parser = mHeaders[i]->getParserCategory();
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
SipMessage::addSource(const sockaddr_in& addr)
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

void
SipMessage::copyFrom(const SipMessage& from)
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

// unknown header interface
StringComponents& 
SipMessage::operator[](const Data& headerName)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (i->first == headerName)
      {
         HeaderFieldValueList* hfvs = i->second;
         if (!i->second->first->isParsed())
         {
            HeaderFieldValue* it = hfvs->first;
            while (it != 0)
            {
               it->mParserCategory = new StringComponent(*it);
            }
            
            hfvs->setParserContainer(new StringComponents(*hfvs));
         }
         return (StringComponents&)*hfvs->getParserCategory();
      }
   }
   
   // create the list with a new component
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   HeaderFieldValue* hfv = new HeaderFieldValue;
   hfv->mParserCategory = new StringComponent(*hfv);
   hfvs->push_back(hfv);
   hfvs->setParserContainer(new StringComponents(*hfvs));
   mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(headerName, hfvs));
   return (StringComponents&)*hfvs->getParserCategory();
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


RequestLineComponent& 
SipMessage::operator[](const RequestLineType& l)
{
   //assert(0); // CJ TODO - Cullen wrote this and I have no clue if it is even
   // close to right

   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
   }
   
   RequestLineComponent* parser = new RequestLineComponent(*mStartLine);
   mStartLine->mParserCategory = parser;
   
   return *parser;
}


StatusLineComponent& 
SipMessage::operator[](const StatusLineType& l)
{
   //assert(0); // CJ TODO - Cullen wrote this and I have no clue if it is even
   // close to right

   if (mStartLine == 0 )
   { 
      mStartLine = new HeaderFieldValue;
   }
   
   StatusLineComponent* parser = new StatusLineComponent(*mStartLine);
   mStartLine->mParserCategory = parser;
   
   return *parser;
}
