#ifndef MessageApi_hxx
#define MessageApi_hxx

#include <list>
#include <utility>

#include <sip2/sipstack/HeaderTypes.hxx>

namespace Vocal2
{

class HeaderFieldValue;

class SipMessage
{
   public:

      SipMessage(char buff[]);
      SipMessage()
         : nIsExternal(false)
      SipMessage(const SipMessage& message);

      SipMessage* clone() const;

      virtual ~SipMessage();

      bool isExternal() const
      {
         return nIsExternal;
      }

      string encode();

      // known header interface
      template <int T>
      typename Header<T>::Type& operator[](const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         HeaderFieldValueList* hfvs = mHeaders[parserFactory.getValue()]
         // empty?
         if (mHeaders[parserFactory.getValue()] == 0)
         {
            // create the list with a new component
            hfvs = new HeaderFieldValueList();
            newList.push_back(new HeaderFieldValue(parserFactory.createParserCategory()));
            mHeaders[parserFactory.getValue()] = newList;
         }
                  
         // already parsed?
         if (!hfvs->front().isParsed())
         {
            // create the appropriate component -- component parses lazy
            // VIA, e.g., iterates through the HeaderFieldValues and return a
            // collection component
            hfvs->setParserCategory(parserFactory.createParserCategory(*hfvs));
         }

         return *(HeaderType<T>::Type*)mHeaders[parserFactory.getValue()]->getParserCategory();
      }

      template <typename T>
      void remove(const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         delete mHeaders[parserFactory.getValue()];
         mHeaders[parserFactory.getValue()] = 0;
      }

      // note: removeFirst/removeLast through the component 

      // unknown header interface
      StringComponent& operator[](const string& symbol);

      void remove(const string& symbol);

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

      const bool nIsExternal;
      HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      typedef list< pair<string, HeaderFieldValueList*> > UnknownHeaders;
      UnknownHeaders mUnknownHeaders;
};

}

#endif
