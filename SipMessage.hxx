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
         
      virtual SipMessage* clone() const;

      virtual ~SipMessage();

      bool isExternal() const
      {
         return mIsExternal;
      }

      bool isRequest() const;
      bool isResponse() const;

      void addBuffer(char* buf);

      virtual std::ostream& dump(std::ostream& strm) const;
      std::ostream& encode(std::ostream& str) const;


      Data brief() const ;

      template <int T>
      bool
      exists(const Header<T>& headerType)
      {
         return mHeaders[T] != 0;
      }

      template <int T>
      typename Header<T>::Type& 
      get(const Header<T>& headerType)
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

         return *(typename Header<T>::Type*)mHeaders[T]->getParserCategory();
      }

      template <int T>
      ParserContainer<typename MultiHeader<T>::Type>& 
      get(const MultiHeader<T>& headerType)
      {
         HeaderFieldValueList* hfvs = mHeaders[T];
         // empty?
         if (hfvs == 0)
         {
            // create the list with a new component
            hfvs = new HeaderFieldValueList;
            // create an empty parser container
            hfvs->setParserContainer(new ParserContainer<typename Header<T>::Type>(*hfvs));
            mHeaders[T] = hfvs;
         }
                  
         // already parsed?
         if (!hfvs->front()->isParsed())
         {
            // create the child parsers
            hfvs->setParserContainer(new ParserContainer<typename Header<T>::Type>(*hfvs));
         }

         return *(ParserContainer<typename Header<T>::Type>*)mHeaders[T]->getParserCategory();
      }

      RequestLineComponent& 
      get(const RequestLineType& l);

      StatusLineComponent& 
      get(const StatusLineType& l);
      
      template <int T>
      void remove(const Header<T>& headerType)
      {
         HeaderTypeHolder<T> parserFactory;

         delete mHeaders[parserFactory.getValue()];
         mHeaders[parserFactory.getValue()] = 0;
      }

      // note: removeFirst/removeLast through the component 

      // unknown header interface
      StringComponents& get(const Data& symbol);

      void remove(const Data& symbol);

      // note: removeFirst/removeLast through the component 

      // clone method
      
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
      HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;
      UnknownHeaders mUnknownHeaders;
  
      bool mHaveFixedDest;
      Data mFixedDest;
      
#ifndef WIN32 // CJ TODO FIX 
      sockaddr_in mSource;
#endif

      std::vector<char*> mBufferList;
      HeaderFieldValue* mStartLine;
      HeaderFieldValue* mBody;

      Data mTransactionId;  // !jf!
};


}

#endif
