#ifndef SipMessage_hxx
#define SipMessage_hxx

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <list>
#include <vector>
#include <utility>



#include <sipstack/HeaderTypes.hxx>
#include <sipstack/ParameterTypes.hxx>
#include <sipstack/Message.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/ParserContainer.hxx>

namespace Vocal2
{

class HeaderFieldValue;

class SipMessage : public Message
{
   public:
      SipMessage();
      
      SipMessage(const SipMessage& message);

      virtual const Data& getTransactionId() const;
         
      virtual ~SipMessage();

      bool isExternal() const
      {
         return mIsExternal;
      }

      bool isRequest() const;
      bool isResponse() const;

      void addBuffer(char* buf);

      virtual std::ostream& encode(std::ostream& str) const;

      Data brief() const ;

      template <int T>
      bool
      exists(const Header<T>& headerType) const
      {
         return mHeaders[T] != 0;
      }

      template <int T>
      typename Header<T>::Type& 
      header(const Header<T>& headerType) const
      {
         HeaderFieldValueList* hfvs = mHeaders[T];
         // empty?
         if (hfvs == 0)
         {
            // create the list with a new component
            hfvs = new HeaderFieldValueList;
            HeaderFieldValue* hfv = new HeaderFieldValue;
            hfvs->push_back(hfv);
            mHeaders[T] = hfvs;
         }
                  
         // already parsed?
         if (!hfvs->front()->isParsed())
         {
            hfvs->front()->setParserCategory(new typename Header<T>::Type(hfvs->front()));
            if (hfvs->moreThanOne())
            {
               // WarningLog(<< "Single content header appears multiple times!");
            }
         }

         return *dynamic_cast<typename Header<T>::Type*>(mHeaders[T]->first->getParserCategory());
      }

      template <int T>
      ParserContainer<typename MultiHeader<T>::Type>& 
      header(const MultiHeader<T>& headerType) const
      {
         HeaderFieldValueList* hfvs = mHeaders[T];
         // empty?
         if (hfvs == 0)
         {
            // create the list with a new component
            hfvs = new HeaderFieldValueList;
            // create an empty parser container
            hfvs->setParserContainer(new ParserContainer<typename MultiHeader<T>::Type>(hfvs));
            mHeaders[T] = hfvs;
         }
                  
         // already parsed?
         if (!hfvs->front()->isParsed())
         {
            // create the child parsers
            hfvs->setParserContainer(new ParserContainer<typename MultiHeader<T>::Type>(hfvs));
         }

         return *dynamic_cast<ParserContainer<typename MultiHeader<T>::Type>*>(mHeaders[T]->getParserContainer());
      }

      RequestLine& 
      header(const RequestLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l) const;
      
      template <int T>
      void remove(const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         delete mHeaders[parserFactory.getValue()];
         mHeaders[parserFactory.getValue()] = 0;
      }

      // note: removeFirst/removeLast through the component 

      // unknown header interface
      StringComponents& header(const Data& symbol) const;

      void remove(const Data& symbol);

      void setStartLine(char* start, int len); 
      void setBody(char* start, int len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      void addSource(const sockaddr_in& addr);
      bool hasFixedDest() const;
      Data getFixedDest() const;
      void setFixedDest(const Data& dest);
      void clearFixedDest();


   private:
      void copyFrom(const SipMessage& message);
      void cleanUp();

      // not available
      SipMessage& operator=(const SipMessage&);

      const bool mIsExternal;
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;
      mutable UnknownHeaders mUnknownHeaders;
  
      bool mHaveFixedDest;
      Data mFixedDest;

#ifndef WIN32 // CJ TODO FIX 
      sockaddr_in mSource;
#endif

      std::vector<char*> mBufferList;
      mutable HeaderFieldValue* mStartLine;
      mutable HeaderFieldValue* mBody;

      Data mTransactionId;  // !jf!
};


}

#endif
