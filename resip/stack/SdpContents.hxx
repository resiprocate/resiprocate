#if !defined(RESIP_SDPCONTENTS_HXX)
#define RESIP_SDPCONTENTS_HXX

#include <vector>
#include <list>
#include <iosfwd>
#include <memory>

#include "resip/stack/Contents.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class SdpContents;

class AttributeHelper
{
   public:
      RESIP_HeapCount(AttributeHelper);
      AttributeHelper();
      AttributeHelper(const AttributeHelper& rhs);
      AttributeHelper& operator=(const AttributeHelper& rhs);

      bool exists(const Data& key) const;
      const std::list<Data>& getValues(const Data& key) const;
      EncodeStream& encode(EncodeStream& s) const;
      void parse(ParseBuffer& pb);
      void addAttribute(const Data& key, const Data& value = Data::Empty);
      void clearAttribute(const Data& key);
   private:
      std::list<std::pair<Data, Data> > mAttributeList;  // used to ensure attribute ordering on encode
      HashMap< Data, std::list<Data> > mAttributes;
};

/**
   @ingroup sip_payload
   @brief  Provides an interface to process and generate SDP bodies (MIME content-type application/sdp). 
  * 
  * This class performs both the parsing and generation of SDP.  Most 
  * interaction with the SDP will be through the Session object, 
  * accessible through the session() method.
  *
  **/
class SdpContents : public Contents
{
   public:
      RESIP_HeapCount(SdpContents);
      typedef enum {IP4=1, IP6} AddrType;
      static const SdpContents Empty;

      class Session;
      
      /** @brief  Provides an interface to read and modify SDP
        * bodies.
        * 
        **/
      class Session
      {
         public:
            class Medium;

            /** @brief  parameters for a specific codec are stored in this class.
              * 
              **/
            class Codec
            {
               public:
                  Codec() : mName(), mRate(0), mPayloadType(-1) {}
                  
                  /** @brief full constructor for a Codec.
                    * 
                    *   This constructor allows a rate and optional parameters to be specified.
                    * 
                    * @param name a string identifying the codec
                    * @param rate number of samples/sec
                    * @param parameters optional list of parameters for the codec
                    * @param encodingParameters optional encoding parameters for the codec
                    * 
                    **/
                  Codec(const Data& name, unsigned long rate, const Data& parameters = Data::Empty, const Data& encodingParameters = Data::Empty);
                  
                  /** @brief constructor for a Codec
                    * 
                    *   This constructor allows for payload type and rate parameters to be specified.
                    * 
                    * @param name a string identifying the codec
                    * @param payloadType RTP payload type to associate with this codec
                    * @param rate sample rate of this codec
                    * 
                    **/
                  Codec(const Data& name, int payloadType, int rate=8000);
                  Codec(const Codec& rhs);
                  Codec& operator=(const Codec& codec);

                  void parse(ParseBuffer& pb,
                             const SdpContents::Session::Medium& medium,
                             int payLoadType);
                  void assignFormatParameters(const SdpContents::Session::Medium& medium);

                  /** @brief returns the name of the codec.
                    *  @return name of the codec
                    **/
                  const Data& getName() const;
                  /** @brief returns the name of the codec.
                    *  @return name of the codec
                    **/
                  int getRate() const;

                  /** @brief returns the RTP payload type associated with this codec.
                    * @return RTP payload type
                    **/
                  int payloadType() const {return mPayloadType;}
                  /** @brief returns the RTP payload type associated with this codec.
                    * @return RTP payload type
                    **/
                  int& payloadType() {return mPayloadType;}
                  
                  /** @brief returns the parameters associated with this codec
                    * @return codec parameters
                    **/
                  const Data& parameters() const {return mParameters;}
                  /** @brief returns the parameters associated with this codec
                    * @return codec parameters
                    **/
                  Data& parameters() {return mParameters;}
                  
                  /** @brief returns the encoding parameters associated with this codec
                    * @return encoding parameters
                    **/
                  const Data& encodingParameters() const {return mEncodingParameters;}
                  /** @brief returns the encoding parameters associated with this codec
                    * @return encoding parameters
                    **/
                  Data& encodingParameters() {return mEncodingParameters;}

                  static const Codec ULaw_8000;
                  static const Codec GSM_8000;
                  static const Codec G723_8000;
                  static const Codec ALaw_8000;
                  static const Codec G722_8000;
                  static const Codec CN;
                  static const Codec G729_8000;
                  static const Codec H263;

                  static const Codec TelephoneEvent;
                  static const Codec FrfDialedDigit;

                  typedef HashMap<int, Codec> CodecMap;
                  // "static" payload types as defined in RFC 3551.
                  // Maps payload type (number) to Codec definition.
                  static CodecMap& getStaticCodecs();

                  friend bool operator==(const Codec&, const Codec&);

               private:
                  Data mName;
                  unsigned long mRate;
                  int mPayloadType;
                  Data mParameters;  // Format parameters
                  Data mEncodingParameters;

                  static std::auto_ptr<CodecMap> sStaticCodecs;
                  static bool sStaticCodecsCreated;
                  friend EncodeStream& operator<<(EncodeStream&, const Codec&);
            };

            /** @brief  processes o= lines in SDP
              **/
            class Origin
            {
               public:
                  /** @brief constructor for origin line
                    * 
                    * @param user session user
                    * @param sessionId session ID
                    * @param version session version
                    * @param addr session address type (IP4 or IP6)
                    * @param address IP address of the session
                    * 
                    **/
                  Origin(const Data& user,
                         const UInt64& sessionId,
                         const UInt64& version,
                         AddrType addr,
                         const Data& address);
                  Origin(const Origin& rhs);
                  Origin& operator=(const Origin& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief returns the session ID
                    * @return session ID
                    **/
                  const UInt64& getSessionId() const {return mSessionId;}
                  /** @brief returns the session ID
                    * @return session ID
                    **/
                  UInt64& getSessionId() { return mSessionId; }
                  
                  /** @brief returns the session version
                    * @return session version
                    **/
                  const UInt64& getVersion() const {return mVersion;}
                  /** @brief returns the session version
                    * @return session version
                    **/
                  UInt64& getVersion() { return mVersion; }
                  /** @brief returns the user string for the session
                    * @return user string
                    **/
                  const Data& user() const {return mUser;}
                  /** @brief returns the user string for the session
                    * @return user string
                    **/
                  Data& user() {return mUser;}
                  
                  /** @brief returns the session address type
                    * 
                    * @return address type (IP4 or IP6)
                    **/
                  AddrType getAddressType() const {return mAddrType;}
                  /** @brief returns the session address type
                    * 
                    * @return address type (IP4 or IP6)
                    **/
                  const Data& getAddress() const {return mAddress;}
                  
                  /** @brief set the address for the session
                    * 
                    * @param host IP address to associate with the session
               * @param type type of addressing
                    **/
                  void setAddress(const Data& host, AddrType type = IP4);

               private:
                  Origin();

                  Data mUser;
                  UInt64 mSessionId;
                  UInt64 mVersion;
                  AddrType mAddrType;
                  Data mAddress;

                  friend class Session;
            };

            /** @brief  process e= (email) lines in the SDP
              * 
              **/
            class Email
            {
               public:
                  /** @brief constructor
                    * 
                    * @param address email address
                    * @param freeText string describing the email address
                    * 
                    **/
                  Email(const Data& address,
                        const Data& freeText);

                  Email(const Email& rhs);
                  Email& operator=(const Email& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief returns the email address
                    * 
                    * @return email address
                    **/
                  const Data& getAddress() const {return mAddress;}
                  /** @brief returns the string describing the email address
                    * 
                    * @return string
                    **/
                  const Data& getFreeText() const {return mFreeText;}

               private:
                  Email() {}

                  Data mAddress;
                  Data mFreeText;

                  friend class Session;
            };
            
            /** @brief  process p= (phone number) lines in the SDP
              * 
              **/
            class Phone
            {
               public:
                  /** @brief constructor
                    * 
                    * @param number phone number
                    * @param freeText text string describing the phone number
                    * 
                    **/
                  Phone(const Data& number,
                        const Data& freeText);
                  Phone(const Phone& rhs);
                  Phone& operator=(const Phone& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  
                  /** @brief return the phone number
                    * 
                    * @return phone number
                    **/
                  const Data& getNumber() const {return mNumber;}
                  /** @brief return text string describing the phone number
                    * 
                    * @return text string describing the phone number
                    **/
                  const Data& getFreeText() const {return mFreeText;}

               private:
                  Phone() {}

                  Data mNumber;
                  Data mFreeText;

                  friend class Session;
            };

            /** @brief  Process c= (connection) lines in SDP
              * 
              * This line specifies the IP address and address type used in the session
              * 
              **/
            class Connection
            {
               public:
                  /** @brief constructor
                    * 
                    * @param addType address type (IP4 or IP6)
                    * @param address IP address
               * @param ttl time to live
                    * 
                    **/
                  Connection(AddrType addType,
                             const Data& address,
                             unsigned long ttl = 0);
                  Connection(const Connection& rhs);
                  Connection& operator=(const Connection& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief returns the connection address type
                    * 
                    * @return address type (IP4 or IP6)
                    **/
                  AddrType getAddressType() const {return mAddrType;}
                  
                  /** @brief returns the connection address
                    * 
                    * @return IP address
                    **/
                  const Data& getAddress() const {return mAddress;}
                  
                  /** @brief set the address for the connection
                    * 
                    * @param host IP address to associate with the connection
               * @param type type of addressing
                    **/
                  void setAddress(const Data& host, AddrType type = IP4);
                  unsigned long ttl() const {return mTTL;}
                  unsigned long& ttl() {return mTTL;}

               private:
                  Connection();

                  AddrType mAddrType;
                  Data mAddress;
                  unsigned long mTTL;

                  friend class Session;
                  friend class Medium;
            };

            /** @brief  Process optional b= (bandwidth) lines in SDP
              * 
              **/
            class Bandwidth
            {
               public:
                  /** @brief Constructor
                    * 
                    * @param modifier alphanumeric word giving the meaning of the bandwidth figure
                    * @param kbPerSecond number of kilobits per second
                    * 
                    **/
                  Bandwidth(const Data& modifier,
                            unsigned long kbPerSecond);
                  Bandwidth(const Bandwidth& rhs);
                  Bandwidth& operator=(const Bandwidth& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief returns the modifier string
                    * 
                    * @return modifier string
                    **/
                  const Data& modifier() const {return mModifier;}
                  /** @brief returns the modifier string
                    * 
                    * @return modifier string
                    **/
                  Data modifier() {return mModifier;}
                  /** @brief returns the number of kilobits/second maximum bandwidth
                    * 
                    * @return maximum bandwidth in kilobits/second
                    **/
                  unsigned long kbPerSecond() const {return mKbPerSecond;}
                  /** @brief returns the number of kilobits/second maximum bandwidth
                    * 
                    * @return maximum bandwidth in kilobits/second
                    **/
                  unsigned long& kbPerSecond() {return mKbPerSecond;}

               private:
                  Bandwidth() {}
                  Data mModifier;
                  unsigned long mKbPerSecond;

                  friend class Session;
                  friend class Medium;
            };

            /** @brief  Process t= (start/stop time) lines in SDP
              * 
              **/
            class Time
            {
               public:
                  /** @brief Constructor
                    * 
                    *   The times given are the decimal part of an NTP timestamp.  To convert these values to UNIX time,
                    * subtract decimal 2208988800 from the value.
                    * 
                    * @param start start time
                    * @param stop stop time
                    * 
                    **/
                  Time(unsigned long start,
                       unsigned long stop);
                  Time(const Time& rhs);
                  Time& operator=(const Time& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief  Repeat time.  Not used for SIP
                    * 
                    **/
                  class Repeat
                  {
                     public:
                        Repeat(unsigned long interval,
                               unsigned long duration,
                               std::list<int> offsets);
                        void parse(ParseBuffer& pb);
                        EncodeStream& encode(EncodeStream&) const;

                        unsigned long getInterval() const {return mInterval;}
                        unsigned long getDuration() const {return mDuration;}
                        const std::list<int> getOffsets() const {return mOffsets;}

                     private:
                        Repeat() {}
                        unsigned long mInterval;
                        unsigned long mDuration;
                        std::list<int> mOffsets;

                        friend class Time;
                  };

                  void addRepeat(const Repeat& repeat);

                  /** @brief return the start time
                    * 
                    * @return start time
                    **/
                  unsigned long getStart() const {return mStart;}
                  /** @brief return the stop time
                    * 
                    * @return stop time
                    **/
                  unsigned long getStop() const {return mStop;}
                  const std::list<Repeat>& getRepeats() const {return mRepeats;}

               private:
                  Time() {}
                  unsigned long mStart;
                  unsigned long mStop;
                  std::list<Repeat> mRepeats;

                  friend class Session;
            };
            
            /** @brief  process z= (timezone) lines
              * 
              * Not used in SIP
              * 
              **/
            class Timezones
            {
               public:
                  /** @brief  specify the time at which a timezone shift will occur and the offset, in seconds
                      @deprecated Unused
                  */
                  class Adjustment
                  {
                     public:
                        Adjustment(unsigned long time,
                                   int offset);
                        Adjustment(const Adjustment& rhs);
                        Adjustment& operator=(const Adjustment& rhs);

                        unsigned long time;
                        int offset;
                  };

                  Timezones();
                  Timezones(const Timezones& rhs);
                  Timezones& operator=(const Timezones& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  void addAdjustment(const Adjustment& adjusment);
                  const std::list<Adjustment>& getAdjustments() const {return mAdjustments; }
               private:
                  std::list<Adjustment> mAdjustments;
            };

            /** @brief  process k= (encryption key) line
              * 
              **/
            class Encryption
            {
               public:
                  typedef enum {NoEncryption = 0, Prompt, Clear, Base64, UriKey} KeyType;
                  Encryption(const KeyType& method,
                             const Data& key);
                  Encryption(const Encryption& rhs);
                  Encryption& operator=(const Encryption& rhs);


                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  const KeyType& getMethod() const {return mMethod;}
                  const KeyType& method() const {return mMethod;}
                  KeyType& method() {return mMethod;}
                  const Data& getKey() const {return mKey;}
                  const Data& key() const {return mKey;}
                  Data& key() {return mKey;}

                  Encryption();
               private:
                  KeyType mMethod;
                  Data mKey;
            };

            /** @brief  process m= (media announcement) blocks
              * 
              **/
            class Medium
            {
               public:
                  Medium();
                  Medium(const Medium& rhs);
                  /** @brief Constructor
                    * 
                    * @param name media type (audio, video, application, data, etc.)
                    * @param port UDP port that will receive RTP
                    * @param multicast a misnomer.  If multicast > 1, the next (multicast)
                    *   even ports will convey RTP and the next (multicast) odd ports will
                    *   convey corresponding RTCP
                    * @param protocol the transport used to convey the media.  Usually "RTP/AVP" for SIP.
                    * 
                    **/
                  Medium(const Data& name,
                         unsigned long port,
                         unsigned long multicast,
                         const Data& protocol);
                  Medium& operator=(const Medium& rhs);

                  void parse(ParseBuffer& pb);
                  EncodeStream& encode(EncodeStream&) const;

                  /** @brief add a format identifier to the m= line
                    * 
                    *   This will need to be called for each codec used in the SDP.
                    * 
                    * @param format format identifier
                    *
                    **/
                  void addFormat(const Data& format);
                  /** @brief set the media connection line.  Optional if main SDP has c= line.
                    * 
                    * @param connection connection line to use
                    **/
                  void setConnection(const Connection& connection);
                  /** @brief add a media connection line.  Optional if main SDP has c= line.
                    * 
                    * @param connection connection line to use
                    **/                  
                  void addConnection(const Connection& connection);
                  void setBandwidth(const Bandwidth& bandwidth);
                  void addBandwidth(const Bandwidth& bandwidth);
                  /** @brief add a media attribute line
                    * 
                    * @param key attribute key
                    * @param value attribute value
                    * 
                    **/
                  void addAttribute(const Data& key, const Data& value = Data::Empty);
                  
                  /** @brief return the media type
                    * 
                    * @return media type  
                    **/
                  const Data& name() const {return mName;}
                  /** @brief return the media type
                    * 
                    * @return media type  
                    **/                  
                  Data& name() {return mName;}
                  /** @brief return the base port
                    * 
                    * @return base port  
                    **/
                  int port() const {return mPort;}
                  /** @brief return the base port
                    * 
                    * @return base port  
                    **/
                  unsigned long& port() {return mPort;}
                  /** @brief change the base port
                    * 
                    * @param port new base port
                    *  
                    **/
                  void setPort(int port);
                  /** @brief get the number of transport port pairs
                    * 
                    * @return number of transport port pairs  
                    **/
                  int multicast() const {return mMulticast;}
                  /** @brief get the number of transport port pairs
                    * 
                    * @return number of transport port pairs  
                    **/
                  unsigned long& multicast() {return mMulticast;}
                  /** @brief return the transport protocol
                    * 
                    * @return transport protocol name  
                    **/
                  const Data& protocol() const {return mProtocol;}
                  /** @brief return the transport protocol
                    * 
                    * @return transport protocol name  
                    **/
                  Data& protocol() {return mProtocol;}

                  // preferred codec/format interface
                  typedef std::list<Codec> CodecContainer;
                  /** preferred codec/format interface
                     @note internal storage of formats, rtpmap attributes, and 
                        ftmp attributes are cleared out after codecs() is 
                        called, since they get converted internally as Codec 
                        objects
                  */
                  const CodecContainer& codecs() const;
                  CodecContainer& codecs();
                  
                  /** @brief remove all codecs from SDP **/
                  void clearCodecs();
                  /** @brief add a codec to the m= line
                    * 
                    * @param codec codec to add
                    *  
                    **/
                  void addCodec(const Codec& codec);

                  /** @brief return a list of codec formats
                    * 
                    *   These formats correspond to RTP payload type identifiers
                    * 
                    * @note formats are cleared out and converted in codec 
                    *   objects when codecs() is called
                    * @return list of codec formats  
                    **/
                  const std::list<Data>& getFormats() const {return mFormats;}
                  
                  /** @brief get optional i= (information) line contents
                    * 
                    * @return contents  
                    **/
                  const Data& information() const {return mInformation;}
                  /** @brief get optional i= (information) line contents
                    * 
                    * @return contents  
                    **/
                  Data& information() {return mInformation;}
                  /** @brief get a list of bandwidth lines
                    * 
                    * @return list of Bandwidth objects  
                    **/
                  const std::list<Bandwidth>& bandwidths() const {return mBandwidths;}
                  std::list<Bandwidth>& bandwidths() {return mBandwidths;}

                  /** @brief get a list of Connection objects, including the Session's c= line.
                    * 
                    *   If the media's c= line is empty, use the Session's c= line.
                    * 
                    * @return list of connections  
                    **/
                  const std::list<Connection> getConnections() const;
                  /** @brief get a list of Connection objects from the m= section.  Does not include session c= line.
                    * 
                    * @return list of connections  
                    **/
                  const std::list<Connection>& getMediumConnections() const {return mConnections;}
                  std::list<Connection>& getMediumConnections() {return mConnections;}
                  const Encryption& getEncryption() const {return mEncryption;}
                  const Encryption& encryption() const {return mEncryption;}
                  Encryption& encryption() {return mEncryption;}
                  /** @brief tests if an a= key is present in the media section
                    * 
                    * @param key key to check
                    *
                    * @return true if key exists, false otherwise  
                    **/
                  bool exists(const Data& key) const;
                  /** @brief get the attribute values corresponding to the key
                    * 
                    * @param key key to check
                    *
                    * @return list of values for given key
                    **/
                  const std::list<Data>& getValues(const Data& key) const;
                  /** @brief erase all attributes for a given key
                    * 
                    * @param key key to clear
                    *  
                    **/
                  void clearAttribute(const Data& key);

                  // Search through this mediums codecs to find and return the first match from the passed in list
                  // Note:  The codecList item that matched the codec from the medium is passed back via pMatchingCodec 
                  //        if a non-NULL pointer is passed in.  The codec returned if from this medium.
                  const Codec& findFirstMatchingCodecs(const CodecContainer& codecs, Codec* pMatchingCodec = 0) const;
                  // Search through this mediums codecs to find and return the first match from the passed in medium
                  // Note:  The passed in medium's codec that matched the codec from this medium is passed back 
                  //        via pMatchingCodec if a non-NULL pointer is passed in.  The codec returned if from this medium.
                  const Codec& findFirstMatchingCodecs(const Medium& medium, Codec* pMatchingCodec = 0) const;

                  /** @brief finds the telephone-event codec
                  *
                  * @return telephone-event "codec"
                  **/
                  const Codec& findTelephoneEventPayloadCodec() const;

                  /** @brief finds the telephone-event payload type
                    * 
                    * @return payload type of telephone-event "codec"  
                    **/
                  int findTelephoneEventPayloadType() const;

               private:
                  void setSession(Session* session);
                  Session* mSession;

                  Data mName;
                  unsigned long mPort;
                  unsigned long mMulticast;
                  Data mProtocol;
                  std::list<Data> mFormats;
                  CodecContainer mCodecs;
                  Data mTransport;
                  Data mInformation;
                  std::list<Connection> mConnections;
                  std::list<Bandwidth> mBandwidths;
                  Encryption mEncryption;
                  AttributeHelper mAttributeHelper;

                  bool mRtpMapDone;
                  typedef HashMap<int, Codec> RtpMap;
                  RtpMap mRtpMap;

                  friend class Session;
            };

            /** @brief session constructor
              * 
              *   Create a new session from origin line, version, and session anme
              * 
              * @param version session version
              * @param origin Origin line
              * @param name session name
              * 
              **/
            Session(int version,
                    const Origin& origin,
                    const Data& name);

            Session() : mVersion(0) {}
            Session(const Session& rhs);
            Session& operator=(const Session& rhs);

            void parse(ParseBuffer& pb);
            EncodeStream& encode(EncodeStream&) const;

            /** @brief return session version
              * 
              * @return session version  
              **/
            int version() const {return mVersion;}
            /** @brief return session version
              * 
              * @return session version  
              **/
            int& version() {return mVersion;}
            /** @brief return session Origin line
              * 
              * @return Origin line  
              **/
            const Origin& origin() const {return mOrigin;}
            /** @brief return session Origin line
              * 
              * @return Origin line  
              **/
            Origin& origin() {return mOrigin;}
            /** @brief return session name
              *
              * @return name  
              **/
            const Data& name() const {return mName;}
            /** @brief return session name
              *
              * @return name  
              **/
            Data& name() {return mName;}
            /** @brief return session Information
              *
              * @return Information line  
              **/
            const Data& information() const {return mInformation;}
            /** @brief return session Information
              *
              * @return Information line  
              **/
            Data& information() {return mInformation;}
            /** @brief return session Uri
              *
              * @return Uri line  
              **/
            const Uri& uri() const {return mUri;}
            /** @brief return session Uri
              *
              * @return Uri line  
              **/
            Uri& uri() {return mUri;}
            /** @brief return session Email list
              *
              * @return Email list  
              **/
            const std::list<Email>& getEmails() const {return mEmails;}
            /** @brief return session Phone number list
              *
              * @return Phone number list  
              **/
            const std::list<Phone>& getPhones() const {return mPhones;}
            /** @brief return session Connection
              *
              * @return Connection line  
              **/
            const Connection& connection() const {return mConnection;}
            /** @brief return session Connection
              *
              * @return Connection line  
              **/
            Connection& connection() {return mConnection;} // !dlb! optional?
            /** @brief check if a c= line is present for the session
              * 
              * @return true if c= line is present  
              **/
            bool isConnection() const { return mConnection.mAddress != Data::Empty; }
            /** @brief return session Bandwidth lines
              *
              * @return Bandwidth lines  
              **/
            const std::list<Bandwidth>& bandwidths() const {return mBandwidths;}
            /** @brief return session Bandwidth lines
              *
              * @return Bandwidth lines  
              **/
            std::list<Bandwidth>& bandwidths() {return mBandwidths;}
            /** @brief return session Time lines
              *
              * @return Time lines  
              **/
            const std::list<Time>& getTimes() const {return mTimes;}
            /** @brief return session Time lines
              *
              * @return Time lines  
              **/
            std::list<Time>& getTimes() {return mTimes;}
            const Timezones& getTimezones() const {return mTimezones;}
            const Encryption& getEncryption() const {return mEncryption;}
            const Encryption& encryption() const {return mEncryption;}
            Encryption& encryption() {return mEncryption;}
            typedef std::list<Medium> MediumContainer;
            /** @brief return session Media lines
              *
              * @return Media lines  
              **/
            const MediumContainer& media() const {return mMedia;}
            /** @brief return session Media lines
              *
              * @return Media lines  
              **/
            MediumContainer& media() {return mMedia;}

            /** @brief add an e= (email) line to session
              * 
              * @param email Email line to add
              *  
              **/
            void addEmail(const Email& email);
            /** @brief add a p= (phone number) line to session
              * 
              * @param phone Phone line to add
              *  
              **/
            void addPhone(const Phone& phone);
            /** @brief add a b= (Bandwidth) line to session
              * 
              * @param bandwidth Bandwidth line to add
              *  
              **/
            void addBandwidth(const Bandwidth& bandwidth);
            /** @brief add a t= (Time) line to session
              * 
              * @param t Time line to add
              *  
              **/
            void addTime(const Time& t);
            /** @brief add an m= (Medium) section to session
              * 
              * @param medium Medium section to add
              *  
              **/
            void addMedium(const Medium& medium);
            /** @brief remove all Medium sections from session
              *   
              **/
            void clearMedium() {  mMedia.clear(); }
            /** @brief erase all attributes for a given key
              * 
              * @param key key to clear
              *  
              **/
            void clearAttribute(const Data& key);
            /** @brief add a session attribute line
              * 
              * @param key attribute key
              * @param value attribute value
              * 
              **/
            void addAttribute(const Data& key, const Data& value = Data::Empty);
            /** @brief tests if an a= key is present in the session
              * 
              * @param key key to check
              *
              * @return true if key exists, false otherwise  
              **/
            bool exists(const Data& key) const;
            /** @brief get the attribute values corresponding to the key
              * 
              * @param key key to check
              *
              * @return list of values for given key
              **/
            const std::list<Data>& getValues(const Data& key) const;

         private:
            int mVersion;
            Origin mOrigin;
            Data mName;
            MediumContainer mMedia;

            // applies to all Media where unspecified
            Data mInformation;
            Uri mUri;
            std::list<Email> mEmails;
            std::list<Phone> mPhones;
            Connection mConnection;
            std::list<Bandwidth> mBandwidths;
            std::list<Time> mTimes;
            Timezones mTimezones;
            Encryption mEncryption;
            AttributeHelper mAttributeHelper;

            friend class SdpContents;
      };

      SdpContents();
      SdpContents(const HeaderFieldValue& hfv, const Mime& contentTypes);
      virtual ~SdpContents();

      // !nash! there is no need for overriding copy ctor as every members gets copied
      //SdpContents(const SdpContents& rhs);
      SdpContents& operator=(const SdpContents& rhs);

      /** @brief duplicate an SdpContents object
        * 
        * @return pointer to a new SdpContents object  
        **/
      virtual Contents* clone() const;

      /** @brief get the parsed SDP
        * 
        * @return parsed SDP object  
        **/
      Session& session() {checkParsed(); return mSession;}
      const Session& session() const {checkParsed(); return mSession;}

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);
      static const Mime& getStaticType() ;

      static bool init();

   private:
      SdpContents(const Data& data, const Mime& contentTypes);
      Session mSession;
};

static bool invokeSdpContentsInit = SdpContents::init();

typedef SdpContents::Session::Codec Codec;

bool operator==(const SdpContents::Session::Codec& lhs,
                const SdpContents::Session::Codec& rhs);
bool operator!=(const SdpContents::Session::Codec& lhs,
                const SdpContents::Session::Codec& rhs);

EncodeStream& operator<<(EncodeStream& str, const SdpContents::Session::Codec& codec);

void skipEol(ParseBuffer& pb);

}

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
 */
