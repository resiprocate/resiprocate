#if !defined(SdpMediaLine_hxx)
#define SdpMediaLine_hxx

#include "Sdp.hxx"
#include "SdpCodec.hxx"
#include "SdpCandidate.hxx"
#include "SdpCandidatePair.hxx"
#include <set>

namespace sdpcontainer
{

class SdpMediaLine
{
public:

   typedef enum 
   {
      MEDIA_TYPE_NONE,
      MEDIA_TYPE_UNKNOWN,
      MEDIA_TYPE_AUDIO,          // "audio" - RFC4566
      MEDIA_TYPE_VIDEO,          // "video" - RFC4566
      MEDIA_TYPE_TEXT,           // "text" - RFC4566
      MEDIA_TYPE_APPLICATION,    // "application" - RFC4566
      MEDIA_TYPE_MESSAGE         // "message" - RFC4566
   } SdpMediaType;
   static const char* SdpMediaTypeString[];

   typedef enum 
   {
      PROTOCOL_TYPE_NONE,
      PROTOCOL_TYPE_UNKNOWN,
      PROTOCOL_TYPE_UDP,         // "udp" - RFC4566
      PROTOCOL_TYPE_RTP_AVP,     // "RTP/AVP" - RFC4566
      PROTOCOL_TYPE_RTP_SAVP,    // "RTP/SAVP" - RFC4566
      PROTOCOL_TYPE_RTP_SAVPF,   // "RTP/SAVPF" - RFC3711
      PROTOCOL_TYPE_TCP,         // "TCP" - RFC4145
      PROTOCOL_TYPE_TCP_RTP_AVP, // "TCP/RTP/AVP" - RFC4571
      PROTOCOL_TYPE_TCP_TLS,     // "TCP/TLS" - RFC4572
      PROTOCOL_TYPE_UDP_TLS,     // "UDP/TLS" - draft-fischl-mmusic-sdp-dtls-04
      PROTOCOL_TYPE_DCCP_TLS,    // "DCCP/TLS" - draft-fischl-mmusic-sdp-dtls-04
      PROTOCOL_TYPE_DCCP_TLS_RTP_SAVP, // "DCCP/TLS/RTP/SAVP" - draft-fischl-mmusic-sdp-dtls-04
      PROTOCOL_TYPE_UDP_TLS_RTP_SAVP,  // "UDP/TLS/RTP/SAVP" - draft-fischl-mmusic-sdp-dtls-04
      PROTOCOL_TYPE_TCP_TLS_RTP_SAVP   // "TCP/TLS/RTP/SAVP" - draft-fischl-mmusic-sdp-dtls-04
   } SdpTransportProtocolType;     
   static const char* SdpTransportProtocolTypeString[];

   class SdpConnection 
   {
   public:
      SdpConnection(Sdp::SdpNetType netType = Sdp::NET_TYPE_NONE, 
                    Sdp::SdpAddressType addressType = Sdp::ADDRESS_TYPE_NONE, 
                    const char * address = 0, 
                    unsigned int port = 0,
                    unsigned int multicastIpV4Ttl=0) :
         mNetType(netType), mAddressType(addressType), mAddress(address), mPort(port), mMulticastIpV4Ttl(multicastIpV4Ttl) {}
      SdpConnection(const SdpConnection& rhs) :
         mNetType(rhs.mNetType), mAddressType(rhs.mAddressType), mAddress(rhs.mAddress), mPort(rhs.mPort), mMulticastIpV4Ttl(rhs.mMulticastIpV4Ttl) {}

      void setNetType(Sdp::SdpNetType netType) { mNetType = netType; }
      Sdp::SdpNetType getNetType() const { return mNetType; }

      void setAddressType(Sdp::SdpAddressType addressType) { mAddressType = addressType; }
      Sdp::SdpAddressType getAddressType() const { return mAddressType; }

      void setAddress(const char * address) { mAddress = address; }
      const resip::Data& getAddress() const { return mAddress; }

      void setPort(unsigned int port) { mPort = port; }
      unsigned int getPort() const { return mPort; }

      void setMulticastIpV4Ttl(unsigned int multicastIpV4Ttl) { mMulticastIpV4Ttl = multicastIpV4Ttl; }
      unsigned int getMulticastIpV4Ttl() const { return mMulticastIpV4Ttl; }

   private:
      Sdp::SdpNetType      mNetType;
      Sdp::SdpAddressType  mAddressType;
      resip::Data          mAddress;
      unsigned int         mPort;
      unsigned int         mMulticastIpV4Ttl;
   };

   typedef enum 
   {
      ENCRYPTION_METHOD_NONE,
      ENCRYPTION_METHOD_CLEAR,   // "clear" - RFC4566
      ENCRYPTION_METHOD_BASE64,  // "base64" - RFC4566
      ENCRYPTION_METHOD_URI,     // "uri" - RFC4566
      ENCRYPTION_METHOD_PROMPT   // "prompt" - RFC4566
   } SdpEncryptionMethod;
   static const char* SdpEncryptionMethodString[];

   typedef enum 
   {
      DIRECTION_TYPE_NONE,
      DIRECTION_TYPE_SENDRECV,   // "sendrecv" - RFC4566
      DIRECTION_TYPE_SENDONLY,   // "sendonly" - RFC4566
      DIRECTION_TYPE_RECVONLY,   // "recvonly" - RFC4566
      DIRECTION_TYPE_INACTIVE    // "inactive" - RFC4566
   } SdpDirectionType;
   static const char* SdpDirectionTypeString[];

   typedef enum 
   {
      ORIENTATION_TYPE_NONE,
      ORIENTATION_TYPE_PORTRAIT, // "portrait" - RFC 4566
      ORIENTATION_TYPE_LANDSCAPE,// "landscape" - RFC 4566
      ORIENTATION_TYPE_SEASCAPE  // "seascape" - RFC 4566
   } SdpOrientationType;
   static const char* SdpOrientationTypeString[];

   typedef enum 
   {
      TCP_SETUP_ATTRIBUTE_NONE,
      TCP_SETUP_ATTRIBUTE_ACTIVE,  // "active" - RFC4145
      TCP_SETUP_ATTRIBUTE_PASSIVE, // "passive" - RFC4145
      TCP_SETUP_ATTRIBUTE_ACTPASS, // "actpass" - RFC4145
      TCP_SETUP_ATTRIBUTE_HOLDCONN // "holdconn" - RFC4145
   } SdpTcpSetupAttribute;
   static const char* SdpTcpSetupAttributeString[];

   typedef enum 
   {
      TCP_CONNECTION_ATTRIBUTE_NONE,
      TCP_CONNECTION_ATTRIBUTE_NEW,      // "new" - RFC4145
      TCP_CONNECTION_ATTRIBUTE_EXISTING  // "existing" - RFC4145
   } SdpTcpConnectionAttribute;
   static const char* SdpTcpConnectionAttributeString[];

   typedef enum 
   {
      CRYPTO_SUITE_TYPE_NONE,
      CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80,   // "AES_CM_128_HMAC_SHA1_80" - RFC4568
      CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32,   // "AES_CM_128_HMAC_SHA1_32" - RFC4568
      CRYPTO_SUITE_TYPE_F8_128_HMAC_SHA1_80        // "F8_128_HMAC_SHA1_80" - RFC4568
   } SdpCryptoSuiteType;
   static const char* SdpCryptoSuiteTypeString[];

   typedef enum 
   {
      CRYPTO_KEY_METHOD_NONE,
      CRYPTO_KEY_METHOD_INLINE  // "inline" - RFC4568
   } SdpCryptoKeyMethod;
   static const char* SdpCryptoKeyMethodString[];

   typedef enum
   {
      CRYPTO_SRTP_FEC_ORDER_NONE,
      CRYPTO_SRTP_FEC_ORDER_FEC_SRTP,              // "FEC_SRTP" - RFC 4568
      CRYPTO_SRTP_FEC_ORDER_SRTP_FEC               // "SRTP_FEC" - RFC 2568
   } SdpCryptoSrtpFecOrderType;
   static const char* SdpCryptoSrtpFecOrderTypeString[];

   class SdpCrypto 
   {
   public:
      class SdpCryptoKeyParam 
      {
      public:
         SdpCryptoKeyParam(SdpCryptoKeyMethod keyMethod=SdpMediaLine::CRYPTO_KEY_METHOD_NONE, const char * keyValue=0, unsigned int srtpLifetime=0, unsigned int srtpMkiValue=0, unsigned int srtpMkiLength=0) :
            mKeyMethod(keyMethod), mKeyValue(keyValue), mSrtpLifetime(srtpLifetime), mSrtpMkiValue(srtpMkiValue), mSrtpMkiLength(srtpMkiLength) {}
         SdpCryptoKeyParam(const SdpCryptoKeyParam& rhs) :
            mKeyMethod(rhs.mKeyMethod), mKeyValue(rhs.mKeyValue), mSrtpLifetime(rhs.mSrtpLifetime), mSrtpMkiValue(rhs.mSrtpMkiValue), mSrtpMkiLength(rhs.mSrtpMkiLength) {}

         void setKeyMethod(SdpCryptoKeyMethod keyMethod) { mKeyMethod = keyMethod; }
         SdpCryptoKeyMethod getKeyMethod() const { return mKeyMethod; }

         void setKeyValue(const char * keyValue) { mKeyValue = keyValue; }
         const resip::Data& getKeyValue() const { return mKeyValue; }

         void setSrtpLifetime(unsigned int srtpLifetime) { mSrtpLifetime = srtpLifetime; }
         unsigned int getSrtpLifetime() const { return mSrtpLifetime; }

         void setSrtpMkiValue(unsigned int srtpMkiValue) { mSrtpMkiValue = srtpMkiValue; }
         unsigned int getSrtpMkiValue() const { return mSrtpMkiValue; }

         void setSrtpMkiLength(unsigned int srtpMkiLength) { mSrtpMkiLength = srtpMkiLength; }
         unsigned int getSrtpMkiLength() const { return mSrtpMkiLength; }

      private:
         SdpCryptoKeyMethod mKeyMethod;
         resip::Data        mKeyValue;  // srtp key-salt or generic key-info
         unsigned int       mSrtpLifetime;
         unsigned int       mSrtpMkiValue;
         unsigned int       mSrtpMkiLength;
      };

      SdpCrypto() : mTag(0), mSuite(SdpMediaLine::CRYPTO_SUITE_TYPE_NONE), mSrtpKdr(0), mEncryptedSrtp(1), mEncryptedSrtcp(1),
         mAuthenticatedSrtp(1), mSrtpFecOrder(SdpMediaLine::CRYPTO_SRTP_FEC_ORDER_FEC_SRTP), mSrtpWsh(0) {}
      SdpCrypto(const SdpCrypto& rSdpCandidatePair);
      ~SdpCrypto() { }

      typedef std::list<SdpCryptoKeyParam> CryptoKeyParamList;
      typedef std::list<resip::Data> GenericSessionParamList;

      SdpCrypto& operator=(const SdpCrypto& rhs);

      void setTag(unsigned int tag) { mTag = tag; }
      unsigned int getTag() const { return mTag; }

      void setSuite(SdpCryptoSuiteType suite) { mSuite = suite; }
      SdpCryptoSuiteType getSuite() const { return mSuite; }

      void addCryptoKeyParam(SdpCryptoKeyMethod keyMethod, const char * keyValue, unsigned int srtpLifetime=0, unsigned int srtpMkiValue=0, unsigned int srtpMkiLength=0)
      { addCryptoKeyParam(SdpCryptoKeyParam(keyMethod, keyValue, srtpLifetime, srtpMkiValue, srtpMkiLength)); }
      void addCryptoKeyParam(const SdpCryptoKeyParam& keyParam) { mCryptoKeyParams.push_back(keyParam); }
      void clearCryptoKeyParams() { mCryptoKeyParams.clear(); }
      const CryptoKeyParamList& getCryptoKeyParams() const { return mCryptoKeyParams; }

      void setSrtpKdr(unsigned int srtpKdr) { mSrtpKdr = srtpKdr; }
      unsigned int getSrtpKdr() const { return mSrtpKdr; }

      void setEncryptedSrtp(bool encryptedSrtp) { mEncryptedSrtp = encryptedSrtp; }
      bool getEncryptedSrtp() const { return mEncryptedSrtp; }

      void setEncryptedSrtcp(bool encryptedSrtcp) { mEncryptedSrtcp = encryptedSrtcp; }
      bool getEncryptedSrtcp() const { return mEncryptedSrtcp; }

      void setAuthenticatedSrtp(bool authenticatedSrtp) { mAuthenticatedSrtp = authenticatedSrtp; }
      bool getAuthenticatedSrtp() const { return mAuthenticatedSrtp; }

      void setSrtpFecOrder(SdpCryptoSrtpFecOrderType srtpFecOrder) { mSrtpFecOrder = srtpFecOrder; }
      SdpCryptoSrtpFecOrderType getSrtpFecOrder() const { return mSrtpFecOrder; }
      static SdpCryptoSrtpFecOrderType getSrtpFecOrderFromString(const char * order);

      void setSrtpFecKey(SdpCryptoKeyMethod keyMethod, const char * keyValue, unsigned int srtpLifetime=0, unsigned int srtpMkiValue=0, unsigned int srtpMkiLength=0) 
      { mSrtpFecKey.setKeyMethod(keyMethod); mSrtpFecKey.setKeyValue(keyValue); mSrtpFecKey.setSrtpLifetime(srtpLifetime); 
        mSrtpFecKey.setSrtpMkiValue(srtpMkiValue); mSrtpFecKey.setSrtpMkiLength(srtpMkiLength); }
      const SdpCryptoKeyParam& getSrtpFecKey() const { return mSrtpFecKey; }

      void setSrtpWsh(unsigned int srtpWsh) { mSrtpWsh = srtpWsh; }
      unsigned int getSrtpWsh() const { return mSrtpWsh; }

      void addGenericSessionParam(const char * sessionParam) { mGenericSessionParams.push_back(sessionParam); }
      void clearGenericSessionParams() { mGenericSessionParams.clear(); }
      const GenericSessionParamList& getGenericSessionParams() const { return mGenericSessionParams; }

   private:
      unsigned int       mTag;         
      SdpCryptoSuiteType mSuite;
      CryptoKeyParamList mCryptoKeyParams;
      unsigned int       mSrtpKdr;
      bool               mEncryptedSrtp;
      bool               mEncryptedSrtcp;
      bool               mAuthenticatedSrtp;
      SdpCryptoSrtpFecOrderType mSrtpFecOrder;
      SdpCryptoKeyParam  mSrtpFecKey;
      unsigned int       mSrtpWsh;
      GenericSessionParamList mGenericSessionParams; 
   };

   typedef enum 
   {
      FINGERPRINT_HASH_FUNC_NONE,
      FINGERPRINT_HASH_FUNC_SHA_1,        // "sha-1" - RFC4572
      FINGERPRINT_HASH_FUNC_SHA_224,      // "sha-224" - RFC4572
      FINGERPRINT_HASH_FUNC_SHA_256,      // "sha-256" - RFC4572
      FINGERPRINT_HASH_FUNC_SHA_384,      // "sha-384" - RFC4572
      FINGERPRINT_HASH_FUNC_SHA_512,      // "sha-512" - RFC4572
      FINGERPRINT_HASH_FUNC_MD5,          // "md5" - RFC4572
      FINGERPRINT_HASH_FUNC_MD2           // "md2" - RFC4572
   } SdpFingerPrintHashFuncType;
   static const char* SdpFingerPrintHashFuncTypeString[];

   typedef enum 
   {
      KEYMANAGEMENT_PROTOCOL_NONE,
      KEYMANAGEMENT_PROTOCOL_MIKEY        // 'mikey' - RFC4567
   } SdpKeyManagementProtocolType;
   static const char* SdpKeyManagementProtocolTypeString[];

   typedef enum 
   {
      PRECONDITION_TYPE_NONE,
      PRECONDITION_TYPE_QOS               // "qos" - RFC3312
   } SdpPreConditionType;
   static const char* SdpPreConditionTypeString[];

   typedef enum 
   {
      PRECONDITION_STRENGTH_MANDATORY,    // "mandatory" - RFC3312
      PRECONDITION_STRENGTH_OPTIONAL,     // "optional" - RFC3312
      PRECONDITION_STRENGTH_NONE,         // "none" - RFC3312
      PRECONDITION_STRENGTH_FAILURE,      // "failure" - RFC3312
      PRECONDITION_STRENGTH_UNKNWOWN      // "unknown" - RFC3312
   } SdpPreConditionStrengthType;
   static const char* SdpPreConditionStrengthTypeString[];

   typedef enum 
   {
      PRECONDITION_STATUS_NONE,
      PRECONDITION_STATUS_E2E,            // "e2e" - RFC3312
      PRECONDITION_STATUS_LOCAL,          // "local" - RFC3312
      PRECONDITION_STATUS_REMOTE,         // "remote" - RFC3312
   } SdpPreConditionStatusType;
   static const char* SdpPreConditionStatusTypeString[];

   typedef enum 
   {
      PRECONDITION_DIRECTION_NONE,        // "none" - RFC3312
      PRECONDITION_DIRECTION_SEND,        // "send" - RFC3312
      PRECONDITION_DIRECTION_RECV,        // "recv" - RFC3312
      PRECONDITION_DIRECTION_SENDRECV,    // "sendrecv" - RFC3312
   } SdpPreConditionDirectionType;
   static const char* SdpPreConditionDirectionTypeString[];

   class SdpPreCondition 
   {
   public:
      SdpPreCondition(SdpPreConditionType type, SdpPreConditionStatusType status, SdpPreConditionDirectionType direction) :
             mType(type), mStatus(status), mDirection(direction) {}
      SdpPreCondition(const SdpPreCondition& rhs) :
             mType(rhs.mType), mStatus(rhs.mStatus), mDirection(rhs.mDirection) {}

      void setType(SdpPreConditionType type) { mType = type; }
      SdpPreConditionType getType() const { return mType; }

      void setStatus(SdpPreConditionStatusType status) { mStatus = status; }
      SdpPreConditionStatusType getStatus() const { return mStatus; }

      void setDirection(SdpPreConditionDirectionType direction) { mDirection = direction; }
      SdpPreConditionDirectionType getDirection() const { return mDirection; }

   private:
      SdpPreConditionType          mType;
      SdpPreConditionStatusType    mStatus;
      SdpPreConditionDirectionType mDirection;
   };

   class SdpPreConditionDesiredStatus : public SdpPreCondition
   {
   public:
      SdpPreConditionDesiredStatus(SdpPreConditionType type, SdpPreConditionStrengthType strength, SdpPreConditionStatusType status, SdpPreConditionDirectionType direction) :
         SdpPreCondition(type, status, direction), mStrength(strength) {}
      SdpPreConditionDesiredStatus(const SdpPreConditionDesiredStatus& rhs) :
         SdpPreCondition(rhs), mStrength(rhs.mStrength) {}

      void setStrength(SdpPreConditionStrengthType strength) { mStrength = strength; }
      SdpPreConditionStrengthType getStrength() const { return mStrength; }

   private:
      SdpPreConditionStrengthType  mStrength;
   };

   class SdpRemoteCandidate 
   {
   public:
      SdpRemoteCandidate(unsigned int componentId, const char * connectionAddress, unsigned int port) :
         mComponentId(componentId), mConnectionAddress(connectionAddress), mPort(port) {}
      SdpRemoteCandidate(const SdpRemoteCandidate& rhs) :
         mComponentId(rhs.mComponentId), mConnectionAddress(rhs.mConnectionAddress), mPort(rhs.mPort) {}

      void setComponentId(unsigned int componentId) { mComponentId = componentId; }
      unsigned int getComponentId() const { return mComponentId; }

      void setConnectionAddress(const char * connectionAddress) { mConnectionAddress = connectionAddress; }
      const resip::Data& getConnectionAddress() const { return mConnectionAddress; }

      void setPort(unsigned int port) { mPort = port; }
      unsigned int getPort() const { return mPort; }

   private:
      unsigned int   mComponentId;
      resip::Data    mConnectionAddress;
      unsigned int   mPort;
   };

   class SdpTransportProtocolCapabilities 
   {
   public:
      SdpTransportProtocolCapabilities(unsigned int id, SdpTransportProtocolType type) :
         mId(id), mType(type) {}
      SdpTransportProtocolCapabilities(const SdpTransportProtocolCapabilities& rhs) :
         mId(rhs.mId), mType(rhs.mType) {}
      void setId(unsigned int id) { mId = id; }
      const unsigned int getId() const { return mId; }

      void setType(SdpTransportProtocolType type) { mType = type; }
      SdpTransportProtocolType getType() const { return mType; }

   private:
      unsigned int mId;
      SdpTransportProtocolType mType;
   };

   class SdpPotentialConfiguration 
   {
   public:
      class ConfigIdItem 
      {
      public:
         ConfigIdItem(unsigned id, bool optional=false) : mId(id), mOptional(optional) {}
         ConfigIdItem(const ConfigIdItem& rhs) : mId(rhs.mId), mOptional(rhs.mOptional) {}

         void setId(unsigned int id) { mId = id; }
         const unsigned int getId() const { return mId; }

         void setOptional(bool optional) { mOptional = optional; }
         const bool getOptional() const { return mOptional; }

      private:
         unsigned int mId;
         bool mOptional;
      };

      SdpPotentialConfiguration(unsigned int id, bool deleteMediaAttributes, bool deleteSessionAttributes, unsigned int transportId) : 
         mId(id), mDeleteMediaAttributes(deleteMediaAttributes), mDeleteSessionAttributes(deleteSessionAttributes), mTransportId(transportId) {}
      SdpPotentialConfiguration(const SdpPotentialConfiguration& rhs) :
         mId(rhs.mId), mDeleteMediaAttributes(rhs.mDeleteMediaAttributes), mDeleteSessionAttributes(rhs.mDeleteSessionAttributes),
         mTransportId(rhs.mTransportId), mAttributeIdList(rhs.mAttributeIdList)  {}

      typedef std::list<ConfigIdItem> ConfigIdList;

      void setId(unsigned int id) { mId = id; }
      const unsigned int getId() const { return mId; }

      void setDeleteMediaAttributes(bool deleteMediaAttributes) { mDeleteMediaAttributes = deleteMediaAttributes; }
      const bool getDeleteMediaAttributes() const { return mDeleteMediaAttributes; }

      void setDeleteSessionAttributes(bool deleteSessionAttributes) { mDeleteSessionAttributes = deleteSessionAttributes; }
      const bool getDeleteSessionAttributes() const { return mDeleteSessionAttributes; }

      void setTransportId(unsigned int transportId) { mTransportId = transportId; }
      const unsigned int getTransportId() const { return mTransportId; }

      void addAttributeId(unsigned int id, bool optional) { addAttributeId(ConfigIdItem(id, optional)); }
      void addAttributeId(const ConfigIdItem& configIdItem) { mAttributeIdList.push_back(configIdItem); }
      void clearAttributeIds() { mAttributeIdList.clear(); }
      const ConfigIdList& getAttributeIds() const { return mAttributeIdList; }

   private:
      unsigned int mId;
      bool mDeleteMediaAttributes;
      bool mDeleteSessionAttributes;
      unsigned int mTransportId;
      ConfigIdList mAttributeIdList;
   };

   SdpMediaLine();
   SdpMediaLine(const SdpMediaLine& rSdpMediaLine);
   virtual ~SdpMediaLine();

   SdpMediaLine& operator=(const SdpMediaLine& rhs);

   void setMediaType(SdpMediaType mediaType) { mMediaType = mediaType; mMediaTypeString = SdpMediaLine::SdpMediaTypeString[mediaType];}
   void setMediaType(const resip::Data& mediaTypeString) { mMediaType = getMediaTypeFromString(mediaTypeString.c_str()); mMediaTypeString = mediaTypeString;} 
   void setTransportProtocolType(SdpTransportProtocolType transportProtocolType) { mTransportProtocolType = transportProtocolType; mTransportProtocolTypeString = SdpMediaLine::SdpTransportProtocolTypeString[transportProtocolType]; }
   void setTransportProtocolType(const resip::Data& transportProtocolTypeString) { mTransportProtocolType = getTransportProtocolTypeFromString(transportProtocolTypeString.c_str()); mTransportProtocolTypeString = transportProtocolTypeString; }

   void addCodec(const SdpCodec& codec) { mCodecs.push_back(codec); }
   void clearCodecs() { mCodecs.clear(); }

   void setTitle(const char * title) { mTitle = title; }

   void addConnection(Sdp::SdpNetType netType, Sdp::SdpAddressType addressType, const char * address, unsigned int port, unsigned int multicastIpV4Ttl=0) 
        { addConnection(SdpConnection(netType, addressType, address, port, multicastIpV4Ttl)); }
   void addConnection(const SdpConnection& connection) { mConnections.push_back(connection); }
   void clearConnections() { mConnections.clear(); }

   void addRtcpConnection(Sdp::SdpNetType netType, Sdp::SdpAddressType addressType, const char * address, unsigned int port, unsigned int multicastIpV4Ttl=0) 
        { addRtcpConnection(SdpConnection(netType, addressType, address, port, multicastIpV4Ttl)); }
   void addRtcpConnection(const SdpConnection& connection) { mRtcpConnections.push_back(connection); }
   void clearRtcpConnections() { mRtcpConnections.clear(); }

   void addBandwidth(Sdp::SdpBandwidthType type, unsigned int bandwidth) { addBandwidth(Sdp::SdpBandwidth(type, bandwidth)); }
   void addBandwidth(const Sdp::SdpBandwidth& sdpBandwidth) { mBandwidths.push_back(sdpBandwidth); }
   void clearBandwidths() { mBandwidths.clear(); }

   void setEncryptionKey(SdpEncryptionMethod method, const char * key) { mEncryptionMethod = method; mEncryptionKey = key; }
   void setDirection(SdpDirectionType direction) { mDirection = direction; }
   void setPacketTime(unsigned int packetTime) { mPacketTime = packetTime; }
   void setMaxPacketTime(unsigned int maxPacketTime) { mMaxPacketTime = maxPacketTime; }
   void setOrientation(SdpOrientationType orientation) { mOrientation = orientation; }
   void setDescriptionLanguage(const char * descriptionLanguage) { mDescriptionLanguage = descriptionLanguage; }
   void setLanguage(const char * language) { mLanguage = language; }
   void setFrameRate(unsigned int frameRate) { mFrameRate = frameRate; }
   void setQuality(unsigned int quality) { mQuality = quality; }

   void setTcpSetupAttribute(SdpTcpSetupAttribute tcpSetupAttribute) { mTcpSetupAttribute = tcpSetupAttribute; }
   void setTcpConnectionAttribute(SdpTcpConnectionAttribute tcpConnectionAttribute) { mTcpConnectionAttribute = tcpConnectionAttribute; }

//   void addCryptoSettings(unsigned int tag, SdpCryptoSuiteType suite, SdpCryptoKeyMethod keyMethod, const char * keyValue) { addCryptoSettings(new SdpCrypto(tag, suite, keyMethod, keyValue)); }
   void addCryptoSettings(const SdpCrypto& crypto) { mCryptos.push_back(crypto); }
   void clearCryptoSettings() { mCryptos.clear(); }

   void setFingerPrint(SdpFingerPrintHashFuncType fingerPrintHashFunction, const char * fingerPrint) { mFingerPrintHashFunction = fingerPrintHashFunction; mFingerPrint = fingerPrint; }
   void setKeyManagementProtocol(SdpKeyManagementProtocolType protocol, const char* data) { mKeyManagementProtocol = protocol; mKeyManagementData = data; }

   void addPreConditionCurrentStatus(SdpPreConditionType type, SdpPreConditionStatusType status, SdpPreConditionDirectionType direction) 
        { addPreConditionCurrentStatus(SdpPreCondition(type, status, direction)); }
   void addPreConditionCurrentStatus(const SdpPreCondition& preCondition) { mPreConditionCurrentStatus.push_back(preCondition); }
   void clearPreConditionCurrentStatus() { mPreConditionCurrentStatus.clear(); }

   void addPreConditionConfirmStatus(SdpPreConditionType type, SdpPreConditionStatusType status, SdpPreConditionDirectionType direction) 
        { addPreConditionConfirmStatus(SdpPreCondition(type, status, direction)); }
   void addPreConditionConfirmStatus(const SdpPreCondition& preCondition) { mPreConditionConfirmStatus.push_back(preCondition); }
   void clearPreConditionConfirmStatus() { mPreConditionConfirmStatus.clear(); }

   void addPreConditionDesiredStatus(SdpPreConditionType type, SdpPreConditionStrengthType strength, SdpPreConditionStatusType status, SdpPreConditionDirectionType direction) 
        { addPreConditionDesiredStatus(SdpPreConditionDesiredStatus(type, strength, status, direction)); }
   void addPreConditionDesiredStatus(const SdpPreConditionDesiredStatus& preConditionDesiredStatus) { mPreConditionDesiredStatus.push_back(preConditionDesiredStatus); }
   void clearPreConditionDesiredStatus() { mPreConditionDesiredStatus.clear(); }

   void setMaximumPacketRate(double maximumPacketRate) { mMaximumPacketRate = maximumPacketRate; }
   void setLabel(const char * label) { mLabel = label; }
   void setIdentificationTag(const char * identificationTag) { mIdentificationTag = identificationTag; }

   void setIceUserFrag(const char * iceUserFrag) { mIceUserFrag = iceUserFrag; }
   void setIcePassword(const char * icePassword) { mIcePassword = icePassword; }

   void addRemoteCandidate(unsigned int componentId, const char * connectionAddress, unsigned int port) { addRemoteCandidate(SdpRemoteCandidate(componentId, connectionAddress, port)); }
   void addRemoteCandidate(const SdpRemoteCandidate& remoteCandidate) { mRemoteCandidates.push_back(remoteCandidate); }
   void clearRemoteCandidates() { mRemoteCandidates.clear(); }

   // Note:  Candidates should be added after m/c line and rtcp information is set, so that the in-use candidate 
   //        can be properly tagged and CandidatePresents flag can be properly set
   void addCandidate(SdpCandidate& candidate);
   void addCandidate(const char * foundation, unsigned int id, SdpCandidate::SdpCandidateTransportType transport, 
                     UInt64 priority, const char * connectionAddress, unsigned int port, 
                     SdpCandidate::SdpCandidateType candidateType, const char * relatedAddress = 0, 
                     unsigned int relatedPort = 0);
   void clearCandidates() { mCandidates.clear(); mRtpCandidatePresent = false; mRtcpCandidatePresent = false; }

   void addCandidatePair(const SdpCandidate& localCandidate, const SdpCandidate& remoteCandidate, SdpCandidatePair::SdpCandidatePairOffererType offerer)
        { addCandidatePair(SdpCandidatePair(localCandidate, remoteCandidate, offerer)); }
   void addCandidatePair(const SdpCandidatePair& sdpCandidatePair) { mCandidatePairs.insert(sdpCandidatePair); }
   void clearCandidatePairs() { mCandidatePairs.clear(); }

   void addPotentialMediaView(const SdpMediaLine& potentialMediaView) { mPotentialMediaViews.push_back(potentialMediaView); }
   void clearPotentialMediaViews() { mPotentialMediaViews.clear(); }

   void setPotentialMediaViewString(const char *potentialMediaViewString) { mPotentialMediaViewString = potentialMediaViewString; }

   void toString(resip::Data& sdpMediaLineString) const;

   typedef std::list<SdpCodec> CodecList;
   typedef std::list<SdpConnection> ConnectionList;
   typedef std::list<SdpCrypto> CryptoList;
   typedef std::list<SdpPreCondition> SdpPreConditionList;
   typedef std::list<SdpPreConditionDesiredStatus> SdpPreConditionDesiredStatusList;
   typedef std::list<SdpRemoteCandidate> SdpRemoteCandidateList;
   typedef std::set<SdpCandidate> SdpCandidateList;
   typedef std::set<SdpCandidatePair> SdpCandidatePairList;
   typedef std::list<SdpMediaLine> SdpMediaLineList;
         
   const SdpMediaType getMediaType() const { return mMediaType; }
   const resip::Data& getMediaTypeString() const { return mMediaTypeString; }
   static SdpMediaType getMediaTypeFromString(const char * type);
   const SdpTransportProtocolType getTransportProtocolType() const { return mTransportProtocolType; }
   const resip::Data& getTransportProtocolTypeString() const { return mTransportProtocolTypeString; }
   static SdpTransportProtocolType getTransportProtocolTypeFromString(const char * type);
   const CodecList& getCodecs() const { return mCodecs; }
   const resip::Data& getTitle() const  { return mTitle; }
   const ConnectionList& getConnections() const { return mConnections; }
   const ConnectionList& getRtcpConnections() const { return mRtcpConnections; }
   const Sdp::BandwidthList& getBandwidths() const { return mBandwidths; }
   SdpEncryptionMethod getEncryptionMethod() const { return mEncryptionMethod; }
   const resip::Data& getEncryptionKey() const { return mEncryptionKey; }   
   SdpDirectionType getDirection() const { return mDirection; }
   unsigned int getPacketTime() const { return mPacketTime; }
   unsigned int getMaxPacketTime() const { return mMaxPacketTime; }
   SdpOrientationType getOrientation() const { return mOrientation; }
   static SdpOrientationType getOrientationTypeFromString(const char * type);
   const resip::Data& getDescriptionLanguage() const { return mDescriptionLanguage; }
   const resip::Data& getLanguage() const { return mLanguage; }
   unsigned int getFrameRate() const { return mFrameRate; }
   unsigned int getQuality() const { return mQuality; }
   SdpTcpSetupAttribute getTcpSetupAttribute() const { return mTcpSetupAttribute; }
   static SdpTcpSetupAttribute getTcpSetupAttributeFromString(const char * attrib);
   SdpTcpConnectionAttribute getTcpConnectionAttribute() const { return mTcpConnectionAttribute; }
   static SdpTcpConnectionAttribute getTcpConnectionAttributeFromString(const char * attrib);
   const CryptoList& getCryptos() const { return mCryptos; }
   static SdpCryptoSuiteType getCryptoSuiteTypeFromString(const char * type);
   static SdpCryptoKeyMethod getCryptoKeyMethodFromString(const char * type);
   SdpFingerPrintHashFuncType getFingerPrintHashFunction() const { return mFingerPrintHashFunction; }
   static SdpFingerPrintHashFuncType getFingerPrintHashFuncTypeFromString(const char * type);
   const resip::Data& getFingerPrint() const { return mFingerPrint; }
   SdpKeyManagementProtocolType getKeyManagementProtocol() const { return mKeyManagementProtocol; }
   static SdpKeyManagementProtocolType getKeyManagementProtocolTypeFromString(const char * type);
   const resip::Data& getKeyManagementData() const { return mKeyManagementData; }
   const SdpPreConditionList& getPreConditionCurrentStatus() const { return mPreConditionCurrentStatus; }
   const SdpPreConditionList& getPreConditionConfirmStatus() const { return mPreConditionConfirmStatus; }
   const SdpPreConditionDesiredStatusList& getPreConditionDesiredStatus() const { return mPreConditionDesiredStatus; }
   static SdpPreConditionType getPreConditionTypeFromString(const char * type);
   static SdpPreConditionStatusType getPreConditionStatusTypeFromString(const char * type);
   static SdpPreConditionDirectionType getPreConditionDirectionTypeFromString(const char * type);
   static SdpPreConditionStrengthType getPreConditionStrengthTypeFromString(const char * type);
   double getMaximumPacketRate() const { return mMaximumPacketRate; }
   const resip::Data& getLabel() const { return mLabel; }
   const resip::Data& getIdentificationTag() const { return mIdentificationTag; }
   const resip::Data& getIceUserFrag() const { return mIceUserFrag; }
   const resip::Data& getIcePassword() const { return mIcePassword; }
   const SdpRemoteCandidateList& getRemoteCandidates() const { return mRemoteCandidates; }
   const SdpCandidateList& getCandidates() const { return mCandidates; }

   const bool isRtcpEnabled() const { return mRtcpConnections.size() > 0; }
   const bool isRtpCandidatePresent() const { return mRtpCandidatePresent; }
   const bool isRtcpCandidatePresent() const { return mRtcpCandidatePresent; }
   const bool isIceSupported() const { return  mRtpCandidatePresent && (!isRtcpEnabled() || mRtcpCandidatePresent); }

   // TODO:  In g++ std::set members are const and cannot be modified, need to update to a new STL type
   const SdpCandidatePairList& getCandidatePairs() const { return mCandidatePairs; }
   SdpCandidatePairList& getCandidatePairs() { return mCandidatePairs; }  // non-const version for manipulation

   const SdpMediaLineList& getPotentialMediaViews() const { return mPotentialMediaViews; }
   const resip::Data& getPotentialMediaViewString() const { return mPotentialMediaViewString; }

private:
   // m=  Note:  port is stored in each connection
   SdpMediaType   mMediaType;
   resip::Data    mMediaTypeString;
   SdpTransportProtocolType mTransportProtocolType;
   resip::Data    mTransportProtocolTypeString;
   CodecList      mCodecs;

   // i=
   resip::Data    mTitle;

   // c=
   ConnectionList mConnections;           // includes port from m- line
   ConnectionList mRtcpConnections;       // a=rtcp <port> [<nettype> <addrtype> <connection addr>] - RFC3605

   // b=
   Sdp::BandwidthList mBandwidths;

   // k=
   SdpEncryptionMethod  mEncryptionMethod;
   resip::Data    mEncryptionKey;

   // a= media level attributes (including defaults copied from session level attributes)
   SdpDirectionType mDirection;           // a=sendrecv, a=sendonly, a=recvonly, a=inactive - RFC4566
   unsigned int   mPacketTime;            // a=ptime:<packet time> in ms - RFC4566
   unsigned int   mMaxPacketTime;         // a=maxptime:<maximum packet time> in ms - RFC4566
   SdpOrientationType mOrientation;       // a=orient:<orientation> - RFC4566
   resip::Data    mDescriptionLanguage;   // a=sdplang:<language tag> - RFC4566
   resip::Data    mLanguage;              // a=lang:<language tag> - RFC4566
   unsigned int   mFrameRate;             // a=framerate:<frame rate> in video frames/sec - RFC4566
   unsigned int   mQuality;               // a=quality:<quality> 0-10 for vidoe (0 is worst, 10 is best) - RFC4566

   SdpTcpSetupAttribute mTcpSetupAttribute;// a=setup:<setup attribute> - RFC4145
   SdpTcpConnectionAttribute mTcpConnectionAttribute; // a=connection:<conn attribute> - RFC4145

   CryptoList     mCryptos;               // a=crypto:<tag> <crypto-suite> <key method>:<keyvalud> [<session-params>] - RFC4568

   SdpFingerPrintHashFuncType mFingerPrintHashFunction; // a=fingerprint:<hash func> <fingerprint> - RFC4572
   resip::Data      mFingerPrint;

   SdpKeyManagementProtocolType mKeyManagementProtocol; // a=key-mgmt:<protocol id> <key mgmt data> - RFC4567   
   resip::Data      mKeyManagementData;

   SdpPreConditionList mPreConditionCurrentStatus; // a=curr:<pre cond type> <status type> <direction tag> - RFC3312
   SdpPreConditionList mPreConditionConfirmStatus; // a=conf:<pre cond type> <status type> <direction tag> - RFC3312 - are multiple allowed?
   SdpPreConditionDesiredStatusList mPreConditionDesiredStatus; // a=des:<pre cond type> <strength tag> <status type> <direction tag> - RFC3312

   double         mMaximumPacketRate;     // a=maxprate:<packetrate> in packets/s - RFC3890
   resip::Data    mLabel;                 // a=label:<label> - RFC4574
   resip::Data    mIdentificationTag;     // a=mid:<id tag> - RFC3388

   // Ice settings
   resip::Data      mIceUserFrag;         // a=ice-ufrag:<ufrag> (min 4 characters) - draft-ietf-mmusic-ice-12
   resip::Data      mIcePassword;         // a=ice-pwd:<password> (min 22 characters) - draft-ietf-mmusic-ice-12
   SdpRemoteCandidateList mRemoteCandidates;    // a=remote-candidates:<component id> <connection address> <port> ... - draft-ietf-mmusic-ice-12
   SdpCandidateList mCandidates;          // a=candidate:<foundation> <component id> <transport> <qvalue> <connection address> 
                                          //             <port> [<candidate type>] [<relay addr>] [<relay port>] 
                                          //             [<ext attrib name> <ext attrib value>] - draft-ietf-mmusic-ice-12
   bool           mRtpCandidatePresent;  
   bool           mRtcpCandidatePresent;
   SdpCandidatePairList mCandidatePairs;       

   // SDP Capabilities Negotiation
   SdpMediaLineList mPotentialMediaViews; // List of Potential Media Configurations
   resip::Data mPotentialMediaViewString; // string that would be used in a=acfg attribute of an answer using this potential view

   friend EncodeStream& operator<<(EncodeStream& strm, const SdpMediaLine& );
};

EncodeStream& operator<<(EncodeStream& strm, const SdpMediaLine& );

} // namespace

#endif  

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
