#include <sipstack/SipMessage.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/HeaderFieldValueList.hxx>

using namespace Vocal2;
using namespace std;

SipMessage::SipMessage(char* buff)
   : mBuff(buff),
     nIsExternal(true),
     mFixedDest(false)
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

SipMessage*
SipMessage::clone() const
{
   // no message buffer
   SipMessage* newMessage = new SipMessage(0);
   newMessage->copyFrom(*this);
   return newMessage;
}

SipMessage::~SipMessage()
{
   cleanUp();
}

void
SipMessage::cleanUp()
{
   delete [] mBuff;
   
   for (int i = 0; i < Headers::MAX_HEADERS; i++)
   {
      delete mHeaders[i];
   }

   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      delete i->second;
   }
}

void
SipMessage::copyFrom(const SipMessage& from)
{
  if(from.hasFixedDest())
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
}

// unknown header interface
Unknowns& 
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
               it->mParserCategory = new Unknown(*it);
            }
            
            hfvs->setParserContainer(new Unknowns(*hfvs));
         }
         return (Unknowns&)hfvs->getParserCategory();
      }
   }
   
   // create the list with a new component
   HeaderFieldValueList* hfvs = new HeaderFieldValueList;
   HeaderFieldValue* hfv = new HeaderFieldValue;
   hfv->mParserCategory = new Unknown(*hfv);
   hfvs->push_back(hfv);
   hfvs->setParserContainer(new Unknowns(*hfvs));
   mUnknownHeaders.push_back(pair<Data, HeaderFieldValueList*>(headerName, hfvs));
   return (Unknowns&)hfvs->getParserCategory();
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
SipMessage::addHeader(int header, char* headerName, int headerLen, 
                         char* start, int len)
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
SipMessage::hasFixedDest()
{
  return mHaveFixedDest;
}


Data
SipMessage::getFixedDest()
{
  return mFixedDest;
  mHaveFixedDest = true;
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
