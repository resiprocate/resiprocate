#ifndef SipMessage_hxx
#define SipMessage_hxx

#include <list>
#include <utility>

#include <sipstack/HeaderTypes.hxx>

namespace Vocal2
{

class HeaderFieldValue;

class SipMessage
{
   public:

      SipMessage(char* buff);
      SipMessage()
         : nIsExternal(false)
      {}
      
      SipMessage(const SipMessage& message);

      SipMessage* clone() const;

      virtual ~SipMessage();

      bool isExternal() const
      {
         return nIsExternal;
      }

      Data encode();

      // known header interface
      template <int T>
      typename Header<T>::Type& 
      operator[](const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         HeaderFieldValueList* hfvs = mHeaders[parserFactory.getValue()];
         // empty?
         if (mHeaders[parserFactory.getValue()] == 0)
         {
            // create the list with a new component
            hfvs = new HeaderFieldValueList;
            HeaderFieldValue* hfv = new HeaderFieldValue;
            hfv->mParserCategory = parserFactory.createParserCategory(hvf);
            hfvs->push_back(hfv);
            if (parserFactory.isMulti())
            {
               hfvs->setParserCategory(parserFactory.createParserCategory(newList));
            }
            mHeaders[parserFactory.getValue()] = hfvs;
         }
                  
         // already parsed?
         if (!hfvs->front().isParsed())
         {
            if (parserFactory.isMulti())
            {
               hfvs->setParserCategory(parserFactory.createParserCategory(*hfvs));
               HeaderFieldValue* it = hfvs->first;
               while (it != 0)
               {
                  it->setParserCategory(parserFactory.createParserCategory(*it));
                  it = it->next;
               }
            }
            else
            {
               hfvs->front().setParserCategory(parserFactory.createParserCategory(hfvs->front()));               
            }
         }

         return *(typename HeaderTypeHolder<T>::Type*)mHeaders[parserFactory.getValue()]->getParserCategory();
      }

      template <int T>
      void remove(const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         delete mHeaders[parserFactory.getValue()];
         mHeaders[parserFactory.getValue()] = 0;
      }

      // note: removeFirst/removeLast through the component 

      // unknown header interface
      Unknowns& operator[](const Data& symbol);

      void remove(const Data& symbol);

      // note: removeFirst/removeLast through the component 

      // clone method
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(int header, char* headerName, int headerLen, 
                     char* start, int len);

   private:
      void copyFrom(const SipMessage& message);
      void cleanUp();

      // not available
      SipMessage& operator=(const SipMessage&);

      char* mBuff;
      const bool nIsExternal;
      HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;
      UnknownHeaders mUnknownHeaders;
};

}

#endif
