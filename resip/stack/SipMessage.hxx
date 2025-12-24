#if !defined(RESIP_SIPMESSAGE_HXX)
#define RESIP_SIPMESSAGE_HXX 

#include <sys/types.h>

#include <list>
#include <vector>
#include <utility>
#include <memory>
#include <iterator>
#include <type_traits>

#include "resip/stack/Contents.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/ParserContainer.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/MessageDecorator.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/WsCookieContext.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/DinkyPool.hxx"
#include "rutil/StlPoolAllocator.hxx"
#include "rutil/Timer.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class Contents;
class ExtensionHeader;
class SecurityAttributes;

/**
   @ingroup resip_crit
   @ingroup message_passing_tu
   @brief Represents a SIP message.

   This is the class that your app will spend the most time working with. This
   is because, in the UA core/Transaction User architecture, the vast majority
   of interaction is carried out through SIP messaging.

   When you get a SipMessage, generally the first thing you want to know is 
   whether it is a request or a response. This is done by calling 
   SipMessage::isRequest() or SipMessage::isResponse().
   
   Next, it is usually important to determine what the SIP method of the message 
   is. This is done by calling SipMessage::method() (this is a convenience 
   function that checks the method of the Request-Line if the message is a 
   request, or the method of the CSeq if a response).
   
   At this point, it may become useful to examine the start-line of the message.

   If the message is a request, you can get the Request-Line (represented by a 
   RequestLine&) by calling SipMessage::header(const RequestLineType&)
   @code
      RequestLine& rLine = sip.header(h_RequestLine);
   @endcode

   If the message is a response, you can get the Status-Line (represented by a StatusLine&) by calling SipMessage::header(const StatusLineType&)
   @code
      StatusLine& sLine = sip.header(h_StatusLine);
   @endcode

   From here, examination of the various headers is in order. The way the 
   underlying code works is very complicated, but fortunately relatively 
   painless to use. For each header type, there is a subclass of HeaderBase, and 
   a SipMessage::header() function that takes a reference to this subclass. On 
   top of this, there is a static instance of each of these subclasses. Examples 
   include h_To, h_From, h_CSeq, h_CallId, h_Routes, h_Contacts, h_RecordRoutes, 
   etc.

   @code
      NameAddr& to = sip.header(h_To);
      NameAddr& from = sip.header(h_From);
      CSeqCategory& cseq = sip.header(h_CSeq);
      CallId& callId = sip.header(h_CallId);
      ParserContainer<NameAddr>& routes = sip.header(h_Routes);
      ParserContainer<NameAddr>& contacts = sip.header(h_Contacts);
      ParserContainer<NameAddr>& rRoutes = sip.header(h_RecordRoutes);
   @endcode

   Generally speaking, the access token is named in a predictable fashion; all 
   non-alphanumeric characters are omitted, the first letter of each word is 
   capitalized, and the name is pluralized if the header is multi-valued (since 
   this stuff is all macro-generated, sometimes this pluralization isn't quite 
   right; h_AllowEventss, h_PAssertedIdentitys).

   When accessing a single-value header, you need to check whether it 
   exists first (unless you want it to be created implicitly). Also, since all
   header field values are lazily parsed (see LazyParser), you'll want to make 
   sure it is well-formed before attempting to access any portion of it.

   @code
      if(sip.exists(h_Event))
      {
         Token& event = sip.header(h_Event);
         if(event.isWellFormed())
         {
            // do stuff with event.
         }
         else
         {
            // complain bitterly
         }
      }
   @endcode

   When accessing a multi-value header, it is important to keep in mind that 
   it can be empty, even if it exists (for example, "Supported: " has a meaning 
   that is distinct from the lack of a Supported header).

   @code
      if(sip.exists(h_Contacts))
      {
         ParserContainer<NameAddr>& contacts = sip.header(h_Contacts);
         if(!contacts.empty())
         {
            NameAddr& frontContact = contacts.front();
            if(frontContact.isWellFormed())
            {
               // do stuff with frontContact
            }
            else
            {
               // complain bitterly
            }
         }
         else
         {
            // complain bitterly
         }
      }
   @endcode

   In some cases, you will need to access header-types that are not natively 
   supported by the stack (ie, don't have an access-token). ExtensionHeader will
   allow you to construct an access-token at runtime that will retrieve the
   header field value as a ParserContainer<StringCategory>. Here's an example:

   @code
      // We need to access the FooBar header field value here.
      static ExtensionHeader h_FooBar("FooBar");
      if(sip.exists(h_FooBar))
      {
         ParserContainer<StringCategory>& fooBars = sip.header(h_FooBar);
      }
   @endcode

*/
class SipMessage : public TransactionMessage
{
   public:
      RESIP_HeapCount(SipMessage);
#ifndef __SUNPRO_CC
      typedef std::list< std::pair<Data, HeaderFieldValueList*>, StlPoolAllocator<std::pair<Data, HeaderFieldValueList*>, PoolBase > > UnknownHeaders;
#else
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;
#endif

      /**
       * @brief List of known SIP headers, searchable by header type.
       *
       * This container provides access to headers of the SIP message that are known to libresiprocate.
       * The headers can be searched by `Headers::Type` enum values and iterated over. The list of headers
       * is unordered, and any modifications of the container may invalidate iterators, pointers and references
       * into the container. The container does not own the `HeaderFieldValueList` objects, which must be
       * explicitly freed on removal (which is handled by `SipMessage`).
       *
       * Elements of the container can be in an "unused" state, which is when the element was removed but
       * not disposed of. "Unused" elements do not participate in the publicly visible set of elements observed
       * by users (e.g. they are not found by searching or exposed during iteration). "Unused" elements can be
       * reused if the user attempts to insert an element with the matching header type. After that, the reused
       * element becomes publicly accessible again.
       */
      class KnownHeaders
      {
         private:
            /// Small index type that is used to map header type ids onto vector elements
            using HeaderIndex = unsigned char;
            /// Header index value that indicates invalid index
            static constexpr HeaderIndex InvalidHeaderIndex = static_cast<HeaderIndex>(-1);
            static_assert(Headers::MAX_HEADERS <= InvalidHeaderIndex, "HeaderIndex type is too small to cover all SIP header types");

         public:
            /// Publicly accessible information about a header
            class HeaderInfo
            {
                  friend class KnownHeaders;

               private:
                  /// Pointer to header values
                  HeaderFieldValueList* mValues;
                  /// Header type. If `Headers::UNKNOWN`, indicates that this entry is in the "unused" state.
                  Headers::Type mType;

               public:
                  constexpr HeaderInfo() noexcept : mValues(nullptr), mType(Headers::UNKNOWN) {}
                  explicit constexpr HeaderInfo(Headers::Type type, HeaderFieldValueList* values = nullptr) noexcept : mValues(values), mType(type) {}

                  /// Returns header type
                  Headers::Type getType() const noexcept { return mType; }

                  /// Returns header values
                  HeaderFieldValueList* getValues() const noexcept { return mValues; }
                  /// Sets header values
                  void setValues(HeaderFieldValueList* values) noexcept { mValues = values; }

               private:
                  /// Sets header type. Only supposed to be used by `KnownHeaders` implementation.
                  void setType(Headers::Type type) noexcept { mType = type; }
            };

         private:
            /// Internal list of header info entries
            using TypedHeaders = std::vector<HeaderInfo, StlPoolAllocator<HeaderInfo, PoolBase> >;

            /// Iterator over used entries in the list
            template<typename AdoptedIterator>
            class UsedIterator
            {
                  friend class KnownHeaders;

               public:
                  using iterator_category = std::forward_iterator_tag;
                  using difference_type = typename std::iterator_traits<AdoptedIterator>::difference_type;
                  using value_type = typename std::iterator_traits<AdoptedIterator>::value_type;
                  using reference = typename std::iterator_traits<AdoptedIterator>::reference;
                  using pointer = typename std::iterator_traits<AdoptedIterator>::pointer;

               private:
                  /// Adopted iterator into the list of headers
                  AdoptedIterator mIterator;
                  /// Iterator to the end of the list of headers
                  AdoptedIterator mEnd;

               public:
                  constexpr UsedIterator() noexcept : mIterator(), mEnd(mIterator) {}
                  UsedIterator(UsedIterator const&) = default;

                  /// Initializing constructor
                  template<
                     typename OtherAdoptedIterator1,
                     typename OtherAdoptedIterator2,
                     typename = typename std::enable_if<
                        std::is_constructible<AdoptedIterator, OtherAdoptedIterator1&&>::value && std::is_constructible<AdoptedIterator, OtherAdoptedIterator2&&>::value
                     >::type
                  >
                  explicit constexpr UsedIterator(OtherAdoptedIterator1&& iter, OtherAdoptedIterator2&& end)
                     noexcept(std::is_nothrow_constructible<AdoptedIterator, OtherAdoptedIterator1&&>::value &&
                        std::is_nothrow_constructible<AdoptedIterator, OtherAdoptedIterator2&&>::value) :
                     mIterator(std::forward<OtherAdoptedIterator1>(iter)),
                     mEnd(std::forward<OtherAdoptedIterator2>(end))
                  {
                  }

                  /// Convering constructor for mutable -> const iterator conversion
                  template<
                     typename OtherAdoptedIterator,
                     typename = typename std::enable_if<
                        !std::is_same<OtherAdoptedIterator, AdoptedIterator>::value && std::is_constructible<AdoptedIterator, OtherAdoptedIterator const&>::value
                     >::type
                  >
                  constexpr UsedIterator(UsedIterator<OtherAdoptedIterator> const& that)
                     noexcept(std::is_nothrow_constructible<AdoptedIterator, OtherAdoptedIterator const&>::value) :
                     mIterator(that.mIterator),
                     mEnd(that.mEnd)
                  {
                  }

                  reference operator*() const noexcept { return *mIterator; }
                  pointer operator->() const noexcept { return mIterator.operator->(); }
                  UsedIterator operator++() { increment(); return *this; }
                  UsedIterator operator++(int) { UsedIterator copy(*this); increment(); return copy; }

                  friend bool operator==(UsedIterator const& left, UsedIterator const& right) noexcept { return left.mIterator == right.mIterator; }
                  friend bool operator!=(UsedIterator const& left, UsedIterator const& right) noexcept { return !(left == right); }

               private:
                  /// Increments the iterator to the next used entry or end of the list
                  void increment() noexcept
                  {
                     ++mIterator;
                     advanceToNextUsedEntry();
                  }

                  /// Advances to the next used entry in the list
                  void advanceToNextUsedEntry() noexcept
                  {
                     while (mIterator != mEnd && mIterator->getType() == Headers::UNKNOWN)
                        ++mIterator;
                  }
            };

         public:
            using iterator = UsedIterator<TypedHeaders::iterator>;
            using const_iterator = UsedIterator<TypedHeaders::const_iterator>;
            using value_type = TypedHeaders::value_type;
            using reference = TypedHeaders::reference;
            using const_reference = TypedHeaders::const_reference;
            using pointer = TypedHeaders::pointer;
            using const_pointer = TypedHeaders::const_pointer;
            using size_type = TypedHeaders::size_type;
            using difference_type = TypedHeaders::difference_type;
            using allocator_type = TypedHeaders::allocator_type;

         private:
            /// Information about headers
            TypedHeaders mHeaders;
            /// Number of non-"unused" elements in the container
            HeaderIndex mSize;
            /// Indices into `mHeaders`. If an element is `InvalidHeaderIndex` it means the corresponding header has no entry in the list.
            HeaderIndex mHeaderIndices[Headers::MAX_HEADERS];

         public:
            KnownHeaders() noexcept : mSize(0) { resetIndices(); }
            explicit KnownHeaders(const allocator_type& alloc)
               noexcept(std::is_nothrow_constructible<TypedHeaders, const allocator_type&>::value) :
               mHeaders(alloc),
               mSize(0)
            {
               resetIndices();
            }

            // Not copyable or moveable since the container elements may be associated with a memory pool
            KnownHeaders(const KnownHeaders&) = delete;
            KnownHeaders& operator=(const KnownHeaders&) = delete;

            /// Returns `true` if the container is empty. Does not consider "unused" elements.
            bool empty() const noexcept { return mSize == 0; }
            /// Returns the number of elements in the container. Does not consider "unused" elements.
            size_type size() const noexcept { return static_cast<size_type>(mSize); }
            /// Clears the container. Does not free `HeaderFieldValueList` objects but clears them and marks as "unused".
            void clear() noexcept;
            /**
             * @brief Clears the container and invokes the dispose function on every element.
             *
             * The `disposer` function will be called on all entries, including "unused" ones. It is expected to free any resources
             * associated with the entry and must not throw.
             */
            template<typename Disposer>
            void clearAndDispose(Disposer&& disposer) noexcept;

            /// Iterator access
            const_iterator cbegin() const noexcept { return begin(); }
            const_iterator cend() const noexcept { return end(); }

            const_iterator begin() const noexcept { const_iterator it(mHeaders.begin(), mHeaders.end()); it.advanceToNextUsedEntry(); return it; }
            const_iterator end() const noexcept { return const_iterator(mHeaders.end(), mHeaders.end()); }

            iterator begin() noexcept { iterator it(mHeaders.begin(), mHeaders.end()); it.advanceToNextUsedEntry(); return it; }
            iterator end() noexcept { return iterator(mHeaders.end(), mHeaders.end()); }

            /// Reserves internal storage for the given number of entries
            void reserve(size_type capacity) { mHeaders.reserve(capacity); }
            /// Returns capacity of the internal storage
            size_type capacity() const noexcept { return mHeaders.capacity(); }

            /// Finds header information, if present in the list. Returns `end()` if not found. Does not produce "unused" entries.
            iterator find(Headers::Type type) noexcept;
            const_iterator find(Headers::Type type) const noexcept { return const_cast<KnownHeaders*>(this)->find(type); }

            /// Erases an element. Does not free the `HeaderFieldValueList` object but clears it and marks as "unused".
            void erase(iterator it) noexcept;

            /**
             * @brief Constructs or reuses a previously erased element for a given header type.
             *
             * If the header with the given type was previously erased and is in the "unused" state, this call reuses this element.
             * Otherwise, the call constructs a new element with the given header type and a pointer to the `HeaderFieldValueList` object
             * returned by the `valuesFactory()` function call.
             *
             * @returns An iterator to the inserted element.
             */
            template<typename ValuesFactory>
            iterator insert(Headers::Type type, ValuesFactory&& valuesFactory);

         private:
            /// Resets all header indices to `InvalidHeaderIndex`
            void resetIndices() noexcept;
      };

      explicit SipMessage(const Tuple *receivedTransport = 0);
      /// @todo .dlb. public, allows pass by value to compile.
      SipMessage(const SipMessage& message);

      /// @todo .dlb. sure would be nice to have overloaded return value here..
      virtual Message* clone() const;

      SipMessage& operator=(const SipMessage& rhs);
      
      /// Returns the transaction id from the branch or if 2543, the computed hash.
      virtual const Data& getTransactionId() const;

      /**
         @brief Calculates an MD5 hash over the Request-URI, To tag (for
         non-INVITE transactions), From tag, Call-ID, CSeq (including
         the method), and top Via header.  The hash is used for
         transaction matching.
      */
      const Data& getRFC2543TransactionId() const;
      void setRFC2543TransactionId(const Data& tid);
      
      virtual ~SipMessage();

      /** @brief Construct a SipMessage object from a string containing a SIP request
          or response.
          
          @param buffer a buffer containing a SIP message
          @param isExternal true for a message generated externally, false otherwise.
          @return constructed SipMessage object
      */
      static SipMessage* make(const Data& buffer, bool isExternal = false);
      void parseAllHeaders();
      
      static bool checkContentLength;

      /**
      @brief Base exception for SipMessage related exceptions
      */
      class Exception final : public BaseException
      {
         public:
            /**
            @brief constructor that records an exception message, the file and the line
            that the exception occured in.
            */
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}
            /**
            @brief returns the class name of the exception instance
            @return the class name of the instance
            */
            const char* name() const noexcept override { return "SipMessage::Exception"; }
      };

      /// Mark message as internally generated
      inline void setFromTU() 
      {
         mIsExternal = false;
      }

      /// Mark message as externally generated
      inline void setFromExternal()
      {
         mIsExternal = true;
      }
      
      /** 
         @brief Check if SipMessage is to be treated as it came off the wire.

         @return true if the message came from an IP interface or if it was 
                 an internally generated response to an internally generated 
                 request (ie: 408), false otherwise.
      */
      inline bool isExternal() const
      {
         return mIsExternal;
      }

      /** 
         @brief Check if SipMessage came off the wire.
      
         @note differs from isExternal(), since isExternal() also returns true 
               for internally generated responses to internally generate requests 
               (ie: 408, etc.).  isFromWire only ever returns true if the message
               actually came off the wire.

         @return true if the message came from an IP interface, false otherwise.
      */
      inline bool isFromWire() const
      {
         return mReceivedTransportTuple.getType() != UNKNOWN_TRANSPORT;
      }
      
      /// @brief Check if SipMessage is a client transaction
      /// @return true if the message is external and is a response or
      /// an internally-generated request.
      virtual bool isClientTransaction() const;
      
      /** @brief Generate a string from the SipMessage object
      
      @return string representation of a SIP message.
      */
      virtual EncodeStream& encode(EncodeStream& str) const;      
      //sipfrags will not output Content Length if there is no body--introduce
      //friendship to hide this?
      virtual EncodeStream& encodeSipFrag(EncodeStream& str) const;
      EncodeStream& encodeEmbedded(EncodeStream& str) const;
      
      virtual EncodeStream& encodeBrief(EncodeStream& str) const;
      EncodeStream& encodeSingleHeader(Headers::Type type, EncodeStream& str) const;

      /// Returns true if message is a request, false otherwise
      inline bool isRequest() const {return mRequest;}
      /// Returns true if message is a response, false otherwise
      inline bool isResponse() const {return mResponse;}
      /// Returns true if message failed to parse, false otherwise      
      inline bool isInvalid() const{return mInvalid;}
      
      /// @brief returns the method type of the message
      /// @see MethodTypes
      resip::MethodTypes method() const;
      /// Returns a string containing the SIP method for the message
      const Data& methodStr() const;
      
      /// Returns a string containing the response reason text
      const resip::Data* getReason() const{return mReason;}
      
      /// Returns the RequestLine.  This is only valid for request messages.
      const RequestLine& 
      header(const RequestLineType& l) const;

      /// Returns the RequestLine.  This is only valid for request messages.
      RequestLine& 
      header(const RequestLineType& l);

      inline const RequestLine& 
      const_header(const RequestLineType& l) const
      {
         return header(l);
      }

      /// Returns the StatusLine.  This is only valid for response messages.
      const StatusLine& 
      header(const StatusLineType& l) const;

      /// Returns the StatusLine.  This is only valid for response messages.
      StatusLine& 
      header(const StatusLineType& l);

      inline const StatusLine& 
      const_header(const StatusLineType& l) const
      {
         return header(l);
      }

      /// Returns true if the given header field is present, false otherwise
      bool exists(const HeaderBase& headerType) const;
      /// Returns true if the header field is present and non-empty, false otherwise
      bool empty(const HeaderBase& headerType) const;
      /// @brief Prevents a header field from being present when the message is prepared
      /// for sending to a transport.  This does not free the memory that was 
      /// used by the header.
      inline void remove(const HeaderBase& headerType)
      {
         remove(headerType.getTypeNum());
      }

      void remove(Headers::Type type);

#define defineHeader(_header, _name, _type, _rfc)                       \
      const H_##_header::Type& header(const H_##_header& headerType) const; \
            H_##_header::Type& header(const H_##_header& headerType); \
      inline const H_##_header::Type& const_header(const H_##_header& headerType) const \
      {\
         return header(headerType);\
      }

      
#define defineMultiHeader(_header, _name, _type, _rfc)                  \
      const H_##_header##s::Type& header(const H_##_header##s& headerType) const; \
            H_##_header##s::Type& header(const H_##_header##s& headerType); \
      inline const H_##_header##s::Type& const_header(const H_##_header##s& headerType) const \
      {\
         return header(headerType);\
      }
      
      defineHeader(ContentDisposition, "Content-Disposition", Token, "RFC 3261");
      defineHeader(ContentEncoding, "Content-Encoding", Token, "RFC 3261");
      defineHeader(MIMEVersion, "Mime-Version", Token, "RFC 3261");
      defineHeader(Priority, "Priority", Token, "RFC 3261");
      defineHeader(Event, "Event", Token, "RFC 3265");
      defineHeader(SubscriptionState, "Subscription-State", Token, "RFC 3265");
      defineHeader(SIPETag, "SIP-ETag", Token, "RFC 3903");
      defineHeader(SIPIfMatch, "SIP-If-Match", Token, "RFC 3903");
      defineHeader(ContentId, "Content-ID", Token, "RFC 2045");
      defineMultiHeader(AllowEvents, "Allow-Events", Token, "RFC 3265");
      defineMultiHeader(Identity, "Identity", StringCategory, "RFC 8224");   // Originally defined in RFC 4474 as a single header, but later modified by RFC8224 to be a multiheader
      defineMultiHeader(AcceptEncoding, "Accept-Encoding", Token, "RFC 3261");
      defineMultiHeader(AcceptLanguage, "Accept-Language", Token, "RFC 3261");
      defineMultiHeader(Allow, "Allow", Token, "RFC 3261");
      defineMultiHeader(ContentLanguage, "Content-Language", Token, "RFC 3261");
      defineMultiHeader(ProxyRequire, "Proxy-Require", Token, "RFC 3261");
      defineMultiHeader(Require, "Require", Token, "RFC 3261");
      defineMultiHeader(Supported, "Supported", Token, "RFC 3261");
      defineMultiHeader(Unsupported, "Unsupported", Token, "RFC 3261");
      defineMultiHeader(SecurityClient, "Security-Client", Token, "RFC 3329");
      defineMultiHeader(SecurityServer, "Security-Server", Token, "RFC 3329");
      defineMultiHeader(SecurityVerify, "Security-Verify", Token, "RFC 3329");
      defineMultiHeader(RequestDisposition, "Request-Disposition", Token, "RFC 3841");
      defineMultiHeader(Reason, "Reason", Token, "RFC 3326");
      defineMultiHeader(Privacy, "Privacy", PrivacyCategory, "RFC 3323");
      defineMultiHeader(PMediaAuthorization, "P-Media-Authorization", Token, "RFC 3313");
      defineHeader(ReferSub, "Refer-Sub", Token, "RFC 4488");
      defineHeader(AnswerMode, "Answer-Mode", Token, "draft-ietf-answermode-04");
      defineHeader(PrivAnswerMode, "Priv-Answer-Mode", Token, "draft-ietf-answermode-04");

      defineMultiHeader(Accept, "Accept", Mime, "RFC 3261");
      defineHeader(ContentType, "Content-Type", Mime, "RFC 3261");

      defineMultiHeader(CallInfo, "Call-Info", GenericUri, "RFC 3261");
      defineMultiHeader(AlertInfo, "Alert-Info", GenericUri, "RFC 3261");
      defineMultiHeader(ErrorInfo, "Error-Info", GenericUri, "RFC 3261");
      defineHeader(IdentityInfo, "Identity-Info", GenericUri, "RFC 4474");

      defineMultiHeader(RecordRoute, "Record-Route", NameAddr, "RFC 3261");
      defineMultiHeader(Route, "Route", NameAddr, "RFC 3261");
      defineMultiHeader(Contact, "Contact", NameAddr, "RFC 3261");
      defineHeader(From, "From", NameAddr, "RFC 3261");
      defineHeader(To, "To", NameAddr, "RFC 3261");
      defineHeader(ReplyTo, "Reply-To", NameAddr, "RFC 3261");
      defineHeader(ReferTo, "Refer-To", NameAddr, "RFC 3515");
      defineHeader(ReferredBy, "Referred-By", NameAddr, "RFC 3892");
      defineMultiHeader(Path, "Path", NameAddr, "RFC 3327");
      defineMultiHeader(AcceptContact, "Accept-Contact", NameAddr, "RFC 3841");
      defineMultiHeader(RejectContact, "Reject-Contact", NameAddr, "RFC 3841");
      defineMultiHeader(PAssertedIdentity, "P-Asserted-Identity", NameAddr, "RFC 3325");
      defineMultiHeader(PPreferredIdentity, "P-Preferred-Identity", NameAddr, "RFC 3325");
      defineHeader(PCalledPartyId, "P-Called-Party-ID", NameAddr, "RFC 3455");
      defineMultiHeader(PAssociatedUri, "P-Associated-URI", NameAddr, "RFC 3455");
      defineMultiHeader(ServiceRoute, "Service-Route", NameAddr, "RFC 3608");
      defineMultiHeader(RemotePartyId, "Remote-Party-ID", NameAddr, "draft-ietf-sip-privacy-04"); // ?bwc? Not in 3323, should we keep?
      defineMultiHeader(HistoryInfo, "History-Info", NameAddr, "RFC 4244");

      defineHeader(ContentTransferEncoding, "Content-Transfer-Encoding", StringCategory, "RFC 1521");
      defineHeader(Organization, "Organization", StringCategory, "RFC 3261");
      defineHeader(SecWebSocketKey, "Sec-WebSocket-Key", StringCategory, "RFC 6455");
      defineHeader(SecWebSocketKey1, "Sec-WebSocket-Key1", StringCategory, "draft-hixie- thewebsocketprotocol-76");
      defineHeader(SecWebSocketKey2, "Sec-WebSocket-Key2", StringCategory, "draft-hixie- thewebsocketprotocol-76");
      defineHeader(Origin, "Origin", StringCategory, "draft-hixie- thewebsocketprotocol-76");
      defineHeader(Host, "Host", StringCategory, "draft-hixie- thewebsocketprotocol-76");
      defineHeader(SecWebSocketAccept, "Sec-WebSocket-Accept", StringCategory, "RFC 6455");
      defineMultiHeader(Cookie, "Cookie", StringCategory, "RFC 6265");
      defineHeader(Server, "Server", StringCategory, "RFC 3261");
      defineHeader(Subject, "Subject", StringCategory, "RFC 3261");
      defineHeader(UserAgent, "User-Agent", StringCategory, "RFC 3261");
      defineHeader(Timestamp, "Timestamp", StringCategory, "RFC 3261");

      defineHeader(ContentLength, "Content-Length", UInt32Category, "RFC 3261");
      defineHeader(MaxForwards, "Max-Forwards", UInt32Category, "RFC 3261");
      defineHeader(MinExpires, "Min-Expires", UInt32Category, "RFC 3261");
      defineHeader(RSeq, "RSeq", UInt32Category, "RFC 3261");

/// @todo !dlb! this one is not quite right -- can have (comment) after field value
      defineHeader(RetryAfter, "Retry-After", UInt32Category, "RFC 3261");
      defineHeader(FlowTimer, "Flow-Timer", UInt32Category, "RFC 5626");

      defineHeader(Expires, "Expires", ExpiresCategory, "RFC 3261");
      defineHeader(SessionExpires, "Session-Expires", ExpiresCategory, "RFC 4028");
      defineHeader(MinSE, "Min-SE", ExpiresCategory, "RFC 4028");

      defineHeader(CallID, "Call-ID", CallID, "RFC 3261");
      defineHeader(Replaces, "Replaces", CallID, "RFC 3891");
      defineHeader(InReplyTo, "In-Reply-To", CallID, "RFC 3261");
      defineHeader(Join, "Join", CallId, "RFC 3911");
      defineHeader(TargetDialog, "Target-Dialog", CallId, "RFC 4538");

      defineHeader(AuthenticationInfo, "Authentication-Info", Auth, "RFC 3261");
      defineMultiHeader(Authorization, "Authorization", Auth, "RFC 3261");
      defineMultiHeader(ProxyAuthenticate, "Proxy-Authenticate", Auth, "RFC 3261");
      defineMultiHeader(ProxyAuthorization, "Proxy-Authorization", Auth, "RFC 3261");
      defineMultiHeader(WWWAuthenticate, "Www-Authenticate", Auth, "RFC 3261");

      defineHeader(CSeq, "CSeq", CSeqCategory, "RFC 3261");
      defineHeader(Date, "Date", DateCategory, "RFC 3261");
      defineMultiHeader(Warning, "Warning", WarningCategory, "RFC 3261");
      defineMultiHeader(Via, "Via", Via, "RFC 3261");
      defineHeader(RAck, "RAck", RAckCategory, "RFC 3262");

      defineMultiHeader(PAccessNetworkInfo, "P-Access-Network-Info", Token, "RFC 7315"); // section 5.4.
      defineHeader(PChargingVector, "P-Charging-Vector", Token, "RFC 3455");
      defineHeader(PChargingFunctionAddresses, "P-Charging-Function-Addresses", Token, "RFC 3455");
      defineMultiHeader(PVisitedNetworkID, "P-Visited-Network-ID", TokenOrQuotedStringCategory, "RFC 3455");

      defineMultiHeader(UserToUser, "User-to-User", TokenOrQuotedStringCategory, "draft-ietf-cuss-sip-uui-17");

      /// unknown header interface
      const StringCategories& header(const ExtensionHeader& symbol) const;
      StringCategories& header(const ExtensionHeader& symbol);
      bool exists(const ExtensionHeader& symbol) const;
      void remove(const ExtensionHeader& symbol);

      /// typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const KnownHeaders& getRawHeaders() const noexcept { return mKnownHeaders; }
      const UnknownHeaders& getRawUnknownHeaders() const noexcept { return mUnknownHeaders; }
      /**
         Return the raw body string (if it exists). The returned HFV
         and its underlying memory is owned by the SipMessage, and may
         be released when this SipMessage is manipulated.

         This is a low-level interface; see getContents() for higher level.
      **/
      const HeaderFieldValue& getRawBody() const noexcept { return mContentsHfv; }

      /**
         Remove any existing body/contents, and (if non-empty)
         set the body to {body}. It makes full copy of the body
         to stored within the SipMessage (since lifetime of
         the message is unpredictable).

         This is a low-level interface; see setContents() for higher level.
      **/
      void setRawBody(const HeaderFieldValue& body);

      /** @brief Retrieves the body of a SIP message.
        * 
        *   In the case of an INVITE request containing SDP, the body would 
        *   be an SdpContents.  For a MESSAGE request, the body may be PlainContents,
        *   CpimContents, or another subclass of Contents.
        * 
        * @return pointer to the contents of the SIP message
        **/
      Contents* getContents() const;
      /// Removes the contents from the message
      std::unique_ptr<Contents> releaseContents();

      /// @brief Set the contents of the message
      /// @param contents to store in the message
      void setContents(const Contents* contents);
      /// @brief Set the contents of the message
      /// @param contents to store in the message
      void setContents(std::unique_ptr<Contents> contents);

      /// @internal transport interface
      void setStartLine(const char* start, int len); 

      void setBody(const char* start, uint32_t len); 
      
      /// Add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      // Returns the source tuple for the transport that the message was received from
      // only makes sense for messages received from the wire.  Differs from Source
      // since it contains the transport bind address instead of the actual source 
      // address.
      const Tuple& getReceivedTransportTuple() const { return mReceivedTransportTuple; }

      /// Set Tuple for transport from whence this message came
      void setReceivedTransportTuple(const Tuple& transportTuple) { mReceivedTransportTuple = transportTuple;}

      // Returns the source tuple that the message was received from
      // only makes sense for messages received from the wire
      void setSource(const Tuple& tuple) { mSource = tuple; }
      /// @brief Returns the source tuple that the message was received from
      /// only makes sense for messages received from the wire
      const Tuple& getSource() const { return mSource; }
      
      /// Used by the stateless interface to specify where to send a request/response
      void setDestination(const Tuple& tuple) { mDestination = tuple; }
      Tuple& getDestination() { return mDestination; }

      void addBuffer(char* buf);

      uint64_t getCreatedTimeMicroSec() const {return mCreatedTime;}

      /// deal with a notion of an "out-of-band" forced target for SIP routing
      void setForceTarget(const Uri& uri);
      void clearForceTarget();
      const Uri& getForceTarget() const;
      bool hasForceTarget() const;

      const Data& getTlsDomain() const { return mTlsDomain; }
      void setTlsDomain(const Data& domain) { mTlsDomain = domain; }

      const std::list<Data>& getTlsPeerNames() const { return mTlsPeerNames; }
      void setTlsPeerNames(const std::list<Data>& tlsPeerNames) { mTlsPeerNames = tlsPeerNames; }

      const CookieList& getWsCookies() const { return mWsCookies; }
      void setWsCookies(const CookieList& wsCookies) { mWsCookies = wsCookies; }

      std::shared_ptr<WsCookieContext> getWsCookieContext() const noexcept { return mWsCookieContext; }
      void setWsCookieContext(std::shared_ptr<WsCookieContext> wsCookieContext) noexcept { mWsCookieContext = wsCookieContext; }

      Data getCanonicalIdentityString() const;
      
      SipMessage& mergeUri(const Uri& source);      

      void setSecurityAttributes(std::unique_ptr<SecurityAttributes>) noexcept;
      const SecurityAttributes* getSecurityAttributes() const noexcept { return mSecurityAttributes.get(); }

      /// @brief Call a MessageDecorator to process the message before it is
      /// sent to the transport
      void addOutboundDecorator(std::unique_ptr<MessageDecorator> md){mOutboundDecorators.push_back(md.release());}
      void clearOutboundDecorators();
      void callOutboundDecorators(const Tuple &src, 
                                    const Tuple &dest,
                                    const Data& sigcompId);
      void rollbackOutboundDecorators();
      void copyOutboundDecoratorsToStackCancel(SipMessage& cancel);
      void copyOutboundDecoratorsToStackFailureAck(SipMessage& ack);
      bool mIsDecorated;

      bool mIsBadAck200;

   protected:
      // !bwc! Removes or zeros all pointers to heap-allocated memory this
      // class owns.
      void clear(bool leaveResponseStuff=false);
      // !bwc! Frees all heap-allocated memory owned.
      void freeMem(bool leaveResponseStuff=false);
      // Clears mHeaders and cleans up memory
      void clearHeaders();
      
      // !bwc! Initializes members. Will not free heap-allocated memory.
      // Will begin by calling clear().
      void init(const SipMessage& rhs);
   
   private:
      void compute2543TransactionHash() const;

      EncodeStream& 
      encode(EncodeStream& str, bool isSipFrag) const;      

      void copyFrom(const SipMessage& message);

      HeaderFieldValueList* ensureHeaders(Headers::Type type);
      HeaderFieldValueList* ensureHeaders(Headers::Type type) const; // throws if not present

      HeaderFieldValueList* ensureHeader(Headers::Type type);
      HeaderFieldValueList* ensureHeader(Headers::Type type) const; // throws if not present

      void throwHeaderMissing(Headers::Type type) const;

      inline HeaderFieldValueList* getEmptyHfvl()
      {
         void* ptr(mPool.allocate(sizeof(HeaderFieldValueList)));
         return new (ptr) HeaderFieldValueList(mPool);
      }

      inline HeaderFieldValueList* getCopyHfvl(const HeaderFieldValueList& hfvl)
      {
         void* ptr(mPool.allocate(sizeof(HeaderFieldValueList)));
         return new (ptr) HeaderFieldValueList(hfvl, mPool);
      }

      inline void freeHfvl(HeaderFieldValueList* hfvl)
      {
         if(hfvl)
         {
            hfvl->~HeaderFieldValueList();
            mPool.deallocate(hfvl);
         }
      }

      template<class T>
      ParserContainer<T>* makeParserContainer()
      {
         void* ptr(mPool.allocate(sizeof(ParserContainer<T>)));
         return new (ptr) ParserContainer<T>(mPool);
      }

      template<class T>
      ParserContainer<T>* makeParserContainer(HeaderFieldValueList* hfvs,
                                             Headers::Type type = Headers::UNKNOWN)
      {
         void* ptr(mPool.allocate(sizeof(ParserContainer<T>)));
         return new (ptr) ParserContainer<T>(hfvs, type, mPool);
      }

      // indicates this message came from the wire or we want it to look like it 
      // came from the wire (ie. internally generated responses to an internally 
      // generated request), set by the Transport and setFromTu and setFromExternal APIs
      bool mIsExternal;

      // Sizing so that average SipMessages don't need to allocate heap memory
      // To profile current sizing, enable DINKYPOOL_PROFILING in SipMessage.cxx 
      // and look for DebugLog message in SipMessage destructor to know when heap
      // allocations are occuring and how much of the pool is used.
      DinkyPool<3732> mPool;

      // raw text corresponding to each typed header (not yet parsed)
      KnownHeaders mKnownHeaders;
      
      // raw text corresponding to each unknown header
      UnknownHeaders mUnknownHeaders;

      // For messages received from the wire, this indicates information about 
      // the transport the message was received on
      Tuple mReceivedTransportTuple;

      // For messages received from the wire, this indicates where it came
      // from. Can be used to get to the Transport and/or reliable Connection
      Tuple mSource;

      // Used by the TU to specify where a message is to go
      Tuple mDestination;
      
      // Raw buffers coming from the Transport. message manages the memory
      std::vector<char*> mBufferList;

      // special case for the first line of message
      StartLine* mStartLine;
      char mStartLineMem[sizeof(RequestLine) > sizeof(StatusLine) ? sizeof(RequestLine) : sizeof(StatusLine)];

      // raw text for the contents (all of them)
      HeaderFieldValue mContentsHfv;

      // lazy parser for the contents
      mutable Contents* mContents;

      // cached value of a hash of the transaction id for a message received
      // from a 2543 sip element. as per rfc3261 see 17.2.3
      mutable Data mRFC2543TransactionId;

      // is a request or response
      bool mRequest;
      bool mResponse;

      bool mInvalid;
      resip::Data* mReason;
      
      uint64_t mCreatedTime;

      // used when next element is a strict router OR 
      // client forces next hop OOB
      Uri* mForceTarget;

      // domain associated with this message for tls cert
      Data mTlsDomain;

      // peers domain associate with this message (MTLS)
      std::list<Data> mTlsPeerNames;

      // cookies associated with this message from the WebSocket Upgrade request
      CookieList mWsCookies;

      // parsed cookie authentication elements associated with this message from the WebSocket Upgrade request
      std::shared_ptr<WsCookieContext> mWsCookieContext;

      std::unique_ptr<SecurityAttributes> mSecurityAttributes;

      std::vector<MessageDecorator*> mOutboundDecorators;

      friend class TransportSelector;
};

}

#undef ensureHeaderTypeUseable
#undef ensureSingleHeader
#undef ensureMultiHeader
#undef defineHeader
#undef defineMultiHeader

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 * vi: set shiftwidth=3 expandtab:
 */
