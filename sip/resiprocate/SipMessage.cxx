#include <sip2/sipstack/SipMessage.hxx>
#include <sip2/sipstack/HeaderFieldValue.hxx>

using namespace Vocal2;

SipMessage::SipMessage(string* buff)
   : nIsExternal(true)
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
   delete [] buff;
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
copyFrom(const SipMessage& from)
{
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

   for (UnknownHeaders::iterator i = from.mUnknownHeaders.begin();
        i != from.mUnknownHeaders.end(); i++)
   {
      mUnknownHeaders.push_back(pair<string, HeaderFieldValue*>(i->first,
                                                                i->second->clone()));
   }
}

// unknown header interface
StringComponent& 
SipMessage::operator[](const string& symbol)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (strncasecmp(i->first.c_str(), headerName, headerLen) == 0)
      {
         if (!i->second->isParsed())
         {
            // will need to deadbeaf the first component on remove, sigh
            unknown->setComponent(new UnknownComponent(unknown));
         }
         return unknown->getComponent();
      }
   }
   
   // not in the unknowns, add it
   HeaderFieldValue* unknown = new HeaderFieldValue(new UnknownComponent(unknown));
   mUnknownHeaders.push_back(pair<string, HeaderFieldValue*>(symbol, unknown));
   return unknown->getComponent();
}

void
SipMessage::remove(const string& symbol)
{
   for (UnknownHeaders::iterator i = mUnknownHeaders.begin();
        i != mUnknownHeaders.end(); i++)
   {
      if (strncasecmp(i->first.c_str(), headerName, headerLen) == 0)
      {
         delete i->second;
         mUnknownHeaders.remove(i);
         return;
      }
   }
}

void
SipMessage::appendHeader(int header, char* headerName, int headerLen, 
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
      hfv->push_back(newHeader);
      mUnknownHeaders.push_back(pair<string, HeaderFieldValueList*>(string(headerName, headerLen),
                                                                    hfvs));
   }
}
