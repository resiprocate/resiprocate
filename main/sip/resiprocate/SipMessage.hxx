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

#include <sipstack/Headers.hxx>
#include <sipstack/Message.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/ParserContainer.hxx>
#include <util/VException.hxx>

namespace Vocal2
{

class SipMessage : public Message
{
   public:
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;

      SipMessage();
      
      SipMessage(const SipMessage& message);

      virtual const Data& getTransactionId() const;
         
      virtual ~SipMessage();

      class Exception : public VException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : VException(msg, file, line) {}

            Data getName() const { return "SipMessage::Exception"; }
      };

      bool isExternal() const
      {
         return mIsExternal;
      }

      bool isRequest() const;
      bool isResponse() const;

      void addBuffer(char* buf);

      virtual std::ostream& encode(std::ostream& str) const;

      Data brief() const;

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

      CSeq_Header::Type& header(const CSeq_Header& headerType) const;
      Call_ID_Header::Type& header(const Call_ID_Header& headerType) const;
      Authentication_Info_Header::Type& header(const Authentication_Info_Header& headerType) const;
      Authorization_Header::Type& header(const Authorization_Header& headerType) const;
      Content_Disposition_Header::Type& header(const Content_Disposition_Header& headerType) const;
      Content_Encoding_Header::Type& header(const Content_Encoding_Header& headerType) const;
      Content_Length_Header::Type& header(const Content_Length_Header& headerType) const;
      Content_Type_Header::Type& header(const Content_Type_Header& headerType) const;
      Date_Header::Type& header(const Date_Header& headerType) const;
      Expires_Header::Type& header(const Expires_Header& headerType) const;
      From_Header::Type& header(const From_Header& headerType) const;
      In_Reply_To_Header::Type& header(const In_Reply_To_Header& headerType) const;
      MIME_Version_Header::Type& header(const MIME_Version_Header& headerType) const;
      Max_Forwards_Header::Type& header(const Max_Forwards_Header& headerType) const;
      Min_Expires_Header::Type& header(const Min_Expires_Header& headerType) const;
      Organization_Header::Type& header(const Organization_Header& headerType) const;
      Priority_Header::Type& header(const Priority_Header& headerType) const;
      Proxy_Authenticate_Header::Type& header(const Proxy_Authenticate_Header& headerType) const;
      Proxy_Authorization_Header::Type& header(const Proxy_Authorization_Header& headerType) const;
      Refer_To_Header::Type& header(const Refer_To_Header& headerType) const;
      Referred_By_Header::Type& header(const Referred_By_Header& headerType) const;
      Replaces_Header::Type& header(const Replaces_Header& headerType) const;
      Reply_To_Header::Type& header(const Reply_To_Header& headerType) const;
      Retry_After_Header::Type& header(const Retry_After_Header& headerType) const;
      Server_Header::Type& header(const Server_Header& headerType) const;
      Subject_Header::Type& header(const Subject_Header& headerType) const;
      Timestamp_Header::Type& header(const Timestamp_Header& headerType) const;
      To_Header::Type& header(const To_Header& headerType) const;
      User_Agent_Header::Type& header(const User_Agent_Header& headerType) const;
      WWW_Authenticate_Header::Type& header(const WWW_Authenticate_Header& headerType) const;
      Warning_Header::Type& header(const Warning_Header& headerType) const;

      ParserContainer<Accept_MultiHeader::Type>& header(const Accept_MultiHeader& headerType) const;
      ParserContainer<Accept_Encoding_MultiHeader::Type>& header(const Accept_Encoding_MultiHeader& headerType) const;
      ParserContainer<Accept_Language_MultiHeader::Type>& header(const Accept_Language_MultiHeader& headerType) const;
      ParserContainer<Alert_Info_MultiHeader::Type>& header(const Alert_Info_MultiHeader& headerType) const;
      ParserContainer<Allow_MultiHeader::Type>& header(const Allow_MultiHeader& headerType) const;
      ParserContainer<Call_Info_MultiHeader::Type>& header(const Call_Info_MultiHeader& headerType) const;
      ParserContainer<Contact_MultiHeader::Type>& header(const Contact_MultiHeader& headerType) const;
      ParserContainer<Content_Language_MultiHeader::Type>& header(const Content_Language_MultiHeader& headerType) const;
      ParserContainer<Error_Info_MultiHeader::Type>& header(const Error_Info_MultiHeader& headerType) const;
      ParserContainer<Proxy_Require_MultiHeader::Type>& header(const Proxy_Require_MultiHeader& headerType) const;
      ParserContainer<Record_Route_MultiHeader::Type>& header(const Record_Route_MultiHeader& headerType) const;
      ParserContainer<Require_MultiHeader::Type>& header(const Require_MultiHeader& headerType) const;
      ParserContainer<Route_MultiHeader::Type>& header(const Route_MultiHeader& headerType) const;
      ParserContainer<Subscription_State_MultiHeader::Type>& header(const Subscription_State_MultiHeader& headerType) const;
      ParserContainer<Supported_MultiHeader::Type>& header(const Supported_MultiHeader& headerType) const;
      ParserContainer<Unsupported_MultiHeader::Type>& header(const Unsupported_MultiHeader& headerType) const;
      ParserContainer<Via_MultiHeader::Type>& header(const Via_MultiHeader& headerType) const;

      RequestLine& 
      header(const RequestLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l) const;
      
      // unknown header interface
      StringCategories& header(const Data& symbol) const;

      void remove(const Data& symbol);

      // typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const UnknownHeaders& getRawUnknownHeaders() const {return mUnknownHeaders;}

      // transport interface
      void setStartLine(const char* start, int len); 
      void setBody(const char* start, int len); 

      
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
      HeaderFieldValueList* ensureHeader(Headers::Type type) const;
      HeaderFieldValueList* ensureHeaders(Headers::Type type) const;

      // not available
      SipMessage& operator=(const SipMessage&);

      const bool mIsExternal;
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];
      mutable UnknownHeaders mUnknownHeaders;
  
      bool mHaveFixedDest;
      Data mFixedDest;

      sockaddr_in mSource;

      // !dlb! hack out with pre-data pointer in buffer for intrusive list?
      std::vector<char*> mBufferList;
      mutable HeaderFieldValue* mStartLine;
      mutable HeaderFieldValue* mBody;

      Data mTransactionId;  // !jf!
      mutable bool mRequest;
      mutable bool mResponse;
};


}

#endif
