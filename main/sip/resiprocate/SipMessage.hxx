#ifndef SipMessage_hxx
#define SipMessage_hxx

#include <util/Socket.hxx>

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

// !dlb!
typedef NameAddr Url;
typedef ParserContainer<NameAddr> Urls;

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

#define USE_METHOD_TEMPLATE
#ifdef USE_METHOD_TEMPLATE
      template <int T>
      bool
      exists(const Header<T>& headerType) const
      {
         return mHeaders[T] != 0;
      }

#ifndef WIN32
      template <int T>
      typename Header<T>::Type& 
      header(const Header<T>& headerType) const
      {
         HeaderFieldValueList* hfvs = ensureHeader(T);

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
#endif

#ifndef WIN32
      template <int T>
      ParserContainer<typename MultiHeader<T>::Type>& 
      header(const MultiHeader<T>& headerType) const
      {
         HeaderFieldValueList *hfvs = ensureHeader(T);
         
         // already parsed?
         if (!hfvs->front()->isParsed())
         {
            // create the child parsers
            hfvs->setParserContainer(new ParserContainer<typename MultiHeader<T>::Type>(hfvs));
         }

         return *dynamic_cast<ParserContainer<typename MultiHeader<T>::Type>*>(mHeaders[T]->getParserContainer());
      }
#endif

      template <int T>
      void remove(const Header<T>& headerType)
      {
         delete mHeaders[T];
         mHeaders[T] = 0;
      }
#else
#include <sipstack/SipMessageExplicit.hxx>
#endif
      RequestLine& 
      header(const RequestLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l) const;
      
      // note: removeFirst/removeLast through the component 

      // unknown header interface
      StringCategories& header(const Data& symbol) const;

      void remove(const Data& symbol);

      void setStartLine(char* start, int len); 
      void setBody(char* start, int len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);


      void setSource(const sockaddr_in& addr);

      bool hasFixedDest() const;
      Data getFixedDest() const;
      void setFixedDest(const Data& dest);
      void clearFixedDest();

   private:
      void copyFrom(const SipMessage& message);
      void cleanUp();
      HeaderFieldValueList* ensureHeader(int type) const;

      // not available
      SipMessage& operator=(const SipMessage&);

      const bool mIsExternal;
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;
      mutable UnknownHeaders mUnknownHeaders;
  
      bool mHaveFixedDest;
      Data mFixedDest;

      sockaddr_in mSource;

      std::vector<char*> mBufferList;
      mutable HeaderFieldValue* mStartLine;
      mutable HeaderFieldValue* mBody;

      Data mTransactionId;  // !jf!
};


}

#endif
