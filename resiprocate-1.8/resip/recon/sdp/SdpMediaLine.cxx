#include "SdpMediaLine.hxx"

using namespace sdpcontainer;

const char* SdpMediaLine::SdpMediaTypeString[] =
{
   "NONE",
   "UNKNOWN",
   "AUDIO",
   "VIDEO",
   "TEXT",
   "APPLICATION",
   "MESSAGE"
};

const char* SdpMediaLine::SdpTransportProtocolTypeString[] =
{
   "NONE",
   "UNKNOWN",
   "UDP",
   "RTP/AVP",
   "RTP/SAVP",
   "RTP/SAVPF",
   "TCP",
   "TCP/RTP/AVP",
   "TCP/TLS",
   "UDP/TLS",
   "DCCP/TLS",
   "DCCP/TLS/RTP/SAVP",
   "UDP/TLS/RTP/SAVP",
   "TCP/TLS/RTP/SAVP"
};

const char* SdpMediaLine::SdpEncryptionMethodString[] =
{
   "NONE",
   "CLEAR",
   "BASE64",
   "URI",
   "PROMPT"
};

const char* SdpMediaLine::SdpDirectionTypeString[] =
{
   "NONE",
   "SENDRECV",
   "SENDONLY",
   "RECVONLY",
   "INACTIVE"
};

const char* SdpMediaLine::SdpOrientationTypeString[] =
{
   "NONE",
   "PORTRAIT",
   "LANDSCAPE",
   "SEASCAPE"
};

const char* SdpMediaLine::SdpTcpSetupAttributeString[] =
{
   "NONE",
   "ACTIVE",
   "PASSIVE",
   "ACTPASS",
   "HOLDCONN"
};

const char* SdpMediaLine::SdpTcpConnectionAttributeString[] =
{
   "NONE",
   "NEW",
   "EXISTING"
};

const char* SdpMediaLine::SdpCryptoSuiteTypeString[] =
{
   "NONE",
   "AES_CM_128_HMAC_SHA1_80",
   "AES_CM_128_HMAC_SHA1_32",
   "F8_128_HMAC_SHA1_80"
};

const char* SdpMediaLine::SdpCryptoKeyMethodString[] =
{
   "NONE",
   "INLINE"
};

const char* SdpMediaLine::SdpCryptoSrtpFecOrderTypeString[] = 
{
   "NONE",
   "FEC_SRTP",
   "SRTP_FEC"
};

const char* SdpMediaLine::SdpFingerPrintHashFuncTypeString[] =
{
   "NONE",
   "SHA-1",
   "SHA-224",
   "SHA-256",
   "SHA-384",
   "SHA-512",
   "MD5",
   "MD2"
};

const char* SdpMediaLine::SdpKeyManagementProtocolTypeString[] =
{
   "NONE",
   "MIKEY"
};

const char* SdpMediaLine::SdpPreConditionTypeString[] =
{
   "NONE",
   "QOS",
};

const char* SdpMediaLine::SdpPreConditionStrengthTypeString[] =
{
   "MANDATORY",
   "OPTIONAL",
   "NONE",
   "FAILURE",
   "UNKNOWN"
};

const char* SdpMediaLine::SdpPreConditionStatusTypeString[] =
{
   "NONE",
   "E2E",
   "LOCAL",
   "REMOTE"
};

const char* SdpMediaLine::SdpPreConditionDirectionTypeString[] =
{
   "NONE",
   "SEND",
   "RECV",
   "SENDRECV"
};

SdpMediaLine::SdpCrypto::SdpCrypto(const SdpMediaLine::SdpCrypto& rhs)
{
   operator=(rhs); 
}

SdpMediaLine::SdpCrypto&
SdpMediaLine::SdpCrypto::operator=(const SdpMediaLine::SdpCrypto& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   // Assign values
   mTag = rhs.mTag;
   mSuite = rhs.mSuite;
   mCryptoKeyParams = rhs.mCryptoKeyParams;
   mSrtpKdr = rhs.mSrtpKdr;
   mEncryptedSrtp = rhs.mEncryptedSrtp;
   mEncryptedSrtcp = rhs.mEncryptedSrtcp;
   mAuthenticatedSrtp = rhs.mAuthenticatedSrtp;
   mSrtpFecOrder = rhs.mSrtpFecOrder;
   mSrtpFecKey = rhs.mSrtpFecKey;
   mSrtpWsh = rhs.mSrtpWsh;
   mGenericSessionParams = rhs.mGenericSessionParams;

   return *this;
}


// Constructor
SdpMediaLine::SdpMediaLine() :
   mMediaType(SdpMediaLine::MEDIA_TYPE_NONE),
   mTransportProtocolType(SdpMediaLine::PROTOCOL_TYPE_NONE),
   mEncryptionMethod(SdpMediaLine::ENCRYPTION_METHOD_NONE),
   mDirection(SdpMediaLine::DIRECTION_TYPE_NONE),
   mPacketTime(0),
   mMaxPacketTime(0),
   mOrientation(SdpMediaLine::ORIENTATION_TYPE_NONE),
   mFrameRate(0),
   mQuality(0),
   mTcpSetupAttribute(SdpMediaLine::TCP_SETUP_ATTRIBUTE_NONE),
   mTcpConnectionAttribute(SdpMediaLine::TCP_CONNECTION_ATTRIBUTE_NONE),
   mFingerPrintHashFunction(SdpMediaLine::FINGERPRINT_HASH_FUNC_NONE),
   mKeyManagementProtocol(SdpMediaLine::KEYMANAGEMENT_PROTOCOL_NONE),
   mMaximumPacketRate(0),
   mRtpCandidatePresent(false),
   mRtcpCandidatePresent(false)
{
}

// Copy constructor
SdpMediaLine::SdpMediaLine(const SdpMediaLine& rhs)
{
   operator=(rhs); 
}

// Destructor
SdpMediaLine::~SdpMediaLine()
{
   clearCandidates();
   clearCandidatePairs();
}

// Assignment operator
SdpMediaLine&
SdpMediaLine::operator=(const SdpMediaLine& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   // Assign values
   mMediaType = rhs.mMediaType;
   mMediaTypeString = rhs.mMediaTypeString;
   mTransportProtocolType = rhs.mTransportProtocolType;
   mTransportProtocolTypeString = rhs.mTransportProtocolTypeString;
   mCodecs = rhs.mCodecs;
   mTitle = rhs.mTitle;
   mConnections = rhs.mConnections;
   mRtcpConnections = rhs.mRtcpConnections;
   mBandwidths = rhs.mBandwidths;
   mEncryptionMethod = rhs.mEncryptionMethod;
   mEncryptionKey = rhs.mEncryptionKey;
   mDirection = rhs.mDirection;
   mPacketTime = rhs.mPacketTime;
   mMaxPacketTime = rhs.mMaxPacketTime;
   mOrientation = rhs.mOrientation;
   mDescriptionLanguage = rhs.mDescriptionLanguage;
   mLanguage = rhs.mLanguage;
   mFrameRate = rhs.mFrameRate;
   mQuality = rhs.mQuality;
   mTcpSetupAttribute = rhs.mTcpSetupAttribute;
   mTcpConnectionAttribute = rhs.mTcpConnectionAttribute;
   mCryptos = rhs.mCryptos;
   mFingerPrintHashFunction = rhs.mFingerPrintHashFunction;
   mFingerPrint = rhs.mFingerPrint;
   mKeyManagementProtocol = rhs.mKeyManagementProtocol;
   mKeyManagementData = rhs.mKeyManagementData;
   mPreConditionCurrentStatus = rhs.mPreConditionCurrentStatus;
   mPreConditionConfirmStatus = rhs.mPreConditionConfirmStatus;
   mPreConditionDesiredStatus = rhs.mPreConditionDesiredStatus;
   mMaximumPacketRate = rhs.mMaximumPacketRate;
   mLabel = rhs.mLabel;
   mIdentificationTag = rhs.mIdentificationTag;
   mIceUserFrag = rhs.mIceUserFrag;
   mIcePassword = rhs.mIcePassword;
   mRemoteCandidates = rhs.mRemoteCandidates;
   mCandidates = rhs.mCandidates;
   mRtpCandidatePresent = rhs.mRtpCandidatePresent;
   mRtcpCandidatePresent = rhs.mRtcpCandidatePresent;
   mCandidatePairs = rhs.mCandidatePairs;

   return *this;
}

void 
SdpMediaLine::addCandidate(SdpCandidate& candidate) 
{ 
   // Check if Candidate is in use (appears on m/c line or rtcp attributes)
   // First check m/c line(s)
   ConnectionList::iterator it = mConnections.begin();
   for(;it != mConnections.end(); it++)
   {
      if(candidate.getPort() == it->getPort() &&
         candidate.getConnectionAddress() == it->getAddress())  
      {
         mRtpCandidatePresent = true;
         candidate.setInUse(true);
         break;
      }
   }

   // Next check Rtcp Info
   if(isRtcpEnabled())        
   {
      it = mRtcpConnections.begin();
      for(;it != mRtcpConnections.end(); it++)
      {
         if(candidate.getPort() == it->getPort() &&
            candidate.getConnectionAddress() == it->getAddress())  
         {
            mRtcpCandidatePresent = true;
            candidate.setInUse(true);
            break;
         }
      }
   }

   mCandidates.insert(candidate);
}

void 
SdpMediaLine::addCandidate(const char * foundation, unsigned int id, SdpCandidate::SdpCandidateTransportType transport, 
                           UInt64 priority, const char * connectionAddress, unsigned int port, 
                           SdpCandidate::SdpCandidateType candidateType, const char * relatedAddress, 
                           unsigned int relatedPort)
{ 
   SdpCandidate t(foundation, id, transport, priority, connectionAddress, port, candidateType, relatedAddress, relatedPort);
   addCandidate(t); 
}

SdpMediaLine::SdpMediaType 
SdpMediaLine::getMediaTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("audio", dataType))
   {
      return MEDIA_TYPE_AUDIO;
   }
   else if(resip::isEqualNoCase("video", dataType))
   {
      return MEDIA_TYPE_VIDEO;
   }
   else if(resip::isEqualNoCase("text", dataType))
   {
      return MEDIA_TYPE_TEXT;
   }
   else if(resip::isEqualNoCase("application", dataType))
   {
      return MEDIA_TYPE_APPLICATION;
   }
   else if(resip::isEqualNoCase("message", dataType))
   {
      return MEDIA_TYPE_MESSAGE;
   }
   else
   {
      return MEDIA_TYPE_UNKNOWN;
   }
}

SdpMediaLine::SdpTransportProtocolType 
SdpMediaLine::getTransportProtocolTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("udp", dataType))
   {
      return PROTOCOL_TYPE_UDP;
   }
   else if(resip::isEqualNoCase("RTP/AVP", dataType))
   {
      return PROTOCOL_TYPE_RTP_AVP;
   }
   else if(resip::isEqualNoCase("RTP/SAVP", dataType))
   {
      return PROTOCOL_TYPE_RTP_SAVP;
   }
   else if(resip::isEqualNoCase("RTP/SAVPF", dataType))
   {
      return PROTOCOL_TYPE_RTP_SAVPF;
   }
   else if(resip::isEqualNoCase("TCP", dataType))
   {
      return PROTOCOL_TYPE_TCP;
   }
   else if(resip::isEqualNoCase("TCP/RTP/AVP", dataType))
   {
      return PROTOCOL_TYPE_TCP_RTP_AVP;
   }
   else if(resip::isEqualNoCase("TCP/TLS", dataType))
   {
      return PROTOCOL_TYPE_TCP_TLS;
   }
   else if(resip::isEqualNoCase("UDP/TLS", dataType))
   {
      return PROTOCOL_TYPE_UDP_TLS;
   }
   else if(resip::isEqualNoCase("DCCP/TLS", dataType))
   {
      return PROTOCOL_TYPE_DCCP_TLS;
   }
   else if(resip::isEqualNoCase("DCCP/TLS/RTP/SAVP", dataType))
   {
      return PROTOCOL_TYPE_DCCP_TLS_RTP_SAVP;
   }
   else if(resip::isEqualNoCase("UDP/TLS/RTP/SAVP", dataType))
   {
      return PROTOCOL_TYPE_UDP_TLS_RTP_SAVP;
   }
   else if(resip::isEqualNoCase("TCP/TLS/RTP/SAVP", dataType))
   {
      return PROTOCOL_TYPE_TCP_TLS_RTP_SAVP;
   }
   else
   {
      return PROTOCOL_TYPE_UNKNOWN;
   }
}

SdpMediaLine::SdpOrientationType 
SdpMediaLine::getOrientationTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("portrait", dataType))
   {
      return ORIENTATION_TYPE_PORTRAIT;
   }
   else if(resip::isEqualNoCase("landscape", dataType))
   {
      return ORIENTATION_TYPE_LANDSCAPE;
   }
   else if(resip::isEqualNoCase("seascape", dataType))
   {
      return ORIENTATION_TYPE_SEASCAPE;
   }
   else
   {
      return ORIENTATION_TYPE_NONE;
   }
}

SdpMediaLine::SdpTcpSetupAttribute 
SdpMediaLine::getTcpSetupAttributeFromString(const char * attrib)
{
   resip::Data dataType(attrib);

   if(resip::isEqualNoCase("active", dataType))
   {
      return TCP_SETUP_ATTRIBUTE_ACTIVE;
   }
   else if(resip::isEqualNoCase("passive", dataType))
   {
      return TCP_SETUP_ATTRIBUTE_PASSIVE;
   }
   else if(resip::isEqualNoCase("actpass", dataType))
   {
      return TCP_SETUP_ATTRIBUTE_ACTPASS;
   }
   else if(resip::isEqualNoCase("holdconn", dataType))
   {
      return TCP_SETUP_ATTRIBUTE_HOLDCONN;
   }
   else
   {
      return TCP_SETUP_ATTRIBUTE_NONE;
   }
}

SdpMediaLine::SdpTcpConnectionAttribute 
SdpMediaLine::getTcpConnectionAttributeFromString(const char * attrib)
{
   resip::Data dataType(attrib);

   if(resip::isEqualNoCase("new", dataType))
   {
      return TCP_CONNECTION_ATTRIBUTE_NEW;
   }
   else if(resip::isEqualNoCase("existing", dataType))
   {
      return TCP_CONNECTION_ATTRIBUTE_EXISTING;
   }
   else
   {
      return TCP_CONNECTION_ATTRIBUTE_NONE;
   }
}

SdpMediaLine::SdpCryptoSuiteType 
SdpMediaLine::getCryptoSuiteTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("AES_CM_128_HMAC_SHA1_80", dataType))
   {
      return CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_80;
   }
   else if(resip::isEqualNoCase("AES_CM_128_HMAC_SHA1_32", dataType))
   {
      return CRYPTO_SUITE_TYPE_AES_CM_128_HMAC_SHA1_32;
   }
   else if(resip::isEqualNoCase("F8_128_HMAC_SHA1_80", dataType))
   {
      return CRYPTO_SUITE_TYPE_F8_128_HMAC_SHA1_80;
   }
   else
   {
      return CRYPTO_SUITE_TYPE_NONE;
   }
}

SdpMediaLine::SdpCryptoKeyMethod 
SdpMediaLine::getCryptoKeyMethodFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("inline", dataType))
   {
      return CRYPTO_KEY_METHOD_INLINE;
   }
   else
   {
      return CRYPTO_KEY_METHOD_NONE;
   }
}

SdpMediaLine::SdpCryptoSrtpFecOrderType 
SdpMediaLine::SdpCrypto::getSrtpFecOrderFromString(const char * order)
{
   resip::Data dataType(order);

   if(resip::isEqualNoCase("FEC_SRTP", dataType))
   {
      return CRYPTO_SRTP_FEC_ORDER_FEC_SRTP;
   }
   else if(resip::isEqualNoCase("SRTP_FEC", dataType))
   {
      return CRYPTO_SRTP_FEC_ORDER_SRTP_FEC;
   }
   else
   {
      return CRYPTO_SRTP_FEC_ORDER_NONE;
   }
}

SdpMediaLine::SdpFingerPrintHashFuncType 
SdpMediaLine::getFingerPrintHashFuncTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("sha-1", dataType))
   {
      return FINGERPRINT_HASH_FUNC_SHA_1;
   }
   else if(resip::isEqualNoCase("sha-224", dataType))
   {
      return FINGERPRINT_HASH_FUNC_SHA_224;
   }
   else if(resip::isEqualNoCase("sha-256", dataType))
   {
      return FINGERPRINT_HASH_FUNC_SHA_256;
   }
   else if(resip::isEqualNoCase("sha-384", dataType))
   {
      return FINGERPRINT_HASH_FUNC_SHA_384;
   }
   else if(resip::isEqualNoCase("sha-512", dataType))
   {
      return FINGERPRINT_HASH_FUNC_SHA_512;
   }
   else if(resip::isEqualNoCase("md5", dataType))
   {
      return FINGERPRINT_HASH_FUNC_MD5;
   }
   else if(resip::isEqualNoCase("md2", dataType))
   {
      return FINGERPRINT_HASH_FUNC_MD2;
   }
   else
   {
      return FINGERPRINT_HASH_FUNC_NONE;
   }
}

SdpMediaLine::SdpKeyManagementProtocolType 
SdpMediaLine::getKeyManagementProtocolTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("mikey", dataType))
   {
      return KEYMANAGEMENT_PROTOCOL_MIKEY;
   }
   else
   {
      return KEYMANAGEMENT_PROTOCOL_NONE;
   }
}

SdpMediaLine::SdpPreConditionType 
SdpMediaLine::getPreConditionTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("qos", dataType))
   {
      return PRECONDITION_TYPE_QOS;
   }
   else
   {
      return PRECONDITION_TYPE_NONE;
   }
}

SdpMediaLine::SdpPreConditionStatusType 
SdpMediaLine::getPreConditionStatusTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("e2e", dataType))
   {
      return PRECONDITION_STATUS_E2E;
   }
   else if(resip::isEqualNoCase("local", dataType))
   {
      return PRECONDITION_STATUS_LOCAL;
   }
   else if(resip::isEqualNoCase("remote", dataType))
   {
      return PRECONDITION_STATUS_REMOTE;
   }
   else
   {
      return PRECONDITION_STATUS_NONE;
   }
}

SdpMediaLine::SdpPreConditionDirectionType 
SdpMediaLine::getPreConditionDirectionTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("send", dataType))
   {
      return PRECONDITION_DIRECTION_SEND;
   }
   else if(resip::isEqualNoCase("recv", dataType))
   {
      return PRECONDITION_DIRECTION_RECV;
   }
   else if(resip::isEqualNoCase("sendrecv", dataType))
   {
      return PRECONDITION_DIRECTION_SENDRECV;
   }
   else
   {
      return PRECONDITION_DIRECTION_NONE;
   }
}

SdpMediaLine::SdpPreConditionStrengthType 
SdpMediaLine::getPreConditionStrengthTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("mandatory", dataType))
   {
      return PRECONDITION_STRENGTH_MANDATORY;
   }
   else if(resip::isEqualNoCase("optional", dataType))
   {
      return PRECONDITION_STRENGTH_OPTIONAL;
   }
   else if(resip::isEqualNoCase("none", dataType))
   {
      return PRECONDITION_STRENGTH_NONE;
   }
   else if(resip::isEqualNoCase("failure", dataType))
   {
      return PRECONDITION_STRENGTH_FAILURE;
   }
   else if(resip::isEqualNoCase("unknown", dataType))
   {
      return PRECONDITION_STRENGTH_UNKNWOWN;
   }
   else
   {
      return PRECONDITION_STRENGTH_NONE;
   }
}

EncodeStream& 
sdpcontainer::operator<<( EncodeStream& strm, const SdpMediaLine& sdpMediaLine)
{
   strm << "MediaLine:" << std::endl
        << "Type: " << sdpMediaLine.mMediaTypeString << std::endl
        << "TransportProtocol: " << sdpMediaLine.mTransportProtocolTypeString << std::endl;

   SdpMediaLine::CodecList::const_iterator itCodec = sdpMediaLine.mCodecs.begin();
   for(;itCodec != sdpMediaLine.mCodecs.end(); itCodec++)
   {
      strm << *itCodec;
   }

   strm << "Title: '" << sdpMediaLine.mTitle << "'" << std::endl;

   SdpMediaLine::ConnectionList::const_iterator itConnection = sdpMediaLine.mConnections.begin();
   for(;itConnection != sdpMediaLine.mConnections.end(); itConnection++)
   {
      strm << "Connection: netType=" << Sdp::SdpNetTypeString[itConnection->getNetType()]
           << ", addrType=" << Sdp::SdpAddressTypeString[itConnection->getAddressType()]
           << ", addr=" << itConnection->getAddress()
           << ", port=" << itConnection->getPort()
           << ", ttl=" << itConnection->getMulticastIpV4Ttl() << std::endl;
   }

   SdpMediaLine::ConnectionList::const_iterator itRtcpConnection = sdpMediaLine.mRtcpConnections.begin();
   for(;itRtcpConnection != sdpMediaLine.mRtcpConnections.end(); itRtcpConnection++)
   {
      strm << "RTCP Connection: netType=" << Sdp::SdpNetTypeString[itRtcpConnection->getNetType()]
           << ", addrType=" << Sdp::SdpAddressTypeString[itRtcpConnection->getAddressType()]
           << ", addr=" << itRtcpConnection->getAddress()
           << ", port=" << itRtcpConnection->getPort()
           << ", ttl=" << itRtcpConnection->getMulticastIpV4Ttl() << std::endl;
   }

   Sdp::BandwidthList::const_iterator itBandwidth = sdpMediaLine.mBandwidths.begin();
   for(;itBandwidth != sdpMediaLine.mBandwidths.end(); itBandwidth++)
   {
      strm << "Bandwidth: type=" << Sdp::SdpBandwidthTypeString[itBandwidth->getType()]
           << ", bandwidth=" << itBandwidth->getBandwidth() << std::endl;
   }

   strm << "Encryption Key: method=" << SdpMediaLine::SdpEncryptionMethodString[sdpMediaLine.mEncryptionMethod] 
        << ", data='" << sdpMediaLine.mEncryptionKey << "'" << std::endl
        << "Direction: " << SdpMediaLine::SdpDirectionTypeString[sdpMediaLine.mDirection] << std::endl
        << "PacketTime: " << sdpMediaLine.mPacketTime << std::endl
        << "MaxPacketTime: " << sdpMediaLine.mMaxPacketTime << std::endl
        << "Orientation: " << SdpMediaLine::SdpOrientationTypeString[sdpMediaLine.mOrientation] << std::endl
        << "DescriptionLanguage: '" << sdpMediaLine.mDescriptionLanguage << "'" << std::endl
        << "Language: '" << sdpMediaLine.mLanguage << "'" << std::endl
        << "FrameRate: " << sdpMediaLine.mFrameRate << std::endl
        << "Quality: " << sdpMediaLine.mQuality << std::endl
        << "TcpSetupAttrib: " << SdpMediaLine::SdpTcpSetupAttributeString[sdpMediaLine.mTcpSetupAttribute] << std::endl
        << "TcpConnectionAttrib: " << SdpMediaLine::SdpTcpConnectionAttributeString[sdpMediaLine.mTcpConnectionAttribute] << std::endl;

   SdpMediaLine::CryptoList::const_iterator itCrypto = sdpMediaLine.mCryptos.begin();
   for(;itCrypto != sdpMediaLine.mCryptos.end(); itCrypto++)
   {
      strm << "Crypto: tag=" << itCrypto->getTag()
           << ", suite=" << SdpMediaLine::SdpCryptoSuiteTypeString[itCrypto->getSuite()];

      SdpMediaLine::SdpCrypto::CryptoKeyParamList::const_iterator itKeyParam = itCrypto->getCryptoKeyParams().begin();
      for(;itKeyParam!=itCrypto->getCryptoKeyParams().end(); itKeyParam++)
      {
         strm << std::endl << "        Key Param: method=" << SdpMediaLine::SdpCryptoKeyMethodString[itKeyParam->getKeyMethod()]
              << ", key=" << itKeyParam->getKeyValue()
              << ", srtpLifetime=" << itKeyParam->getSrtpLifetime()
              << ", srtpMkiValue=" << itKeyParam->getSrtpMkiValue()
              << ", srtpMkiLength=" << itKeyParam->getSrtpMkiLength();
      }

      strm << std::endl << "        kdr=" << itCrypto->getSrtpKdr()
           << ", encryptSrtp=" << itCrypto->getEncryptedSrtp()
           << ", encryptSrtcp=" << itCrypto->getEncryptedSrtcp()
           << ", authSrtp=" << itCrypto->getAuthenticatedSrtp()
           << ", fecOrder=" << SdpMediaLine::SdpCryptoSrtpFecOrderTypeString[itCrypto->getSrtpFecOrder()]
           << ", wsh=" << itCrypto->getSrtpWsh();
      if(itCrypto->getSrtpFecKey().getKeyMethod() != SdpMediaLine::CRYPTO_KEY_METHOD_NONE)
      {
         strm << std::endl << "        fecKeyMethod=" << SdpMediaLine::SdpCryptoKeyMethodString[itCrypto->getSrtpFecKey().getKeyMethod()]
              << ", fecKey=" << itCrypto->getSrtpFecKey().getKeyValue()
              << ", fecLifetime=" << itCrypto->getSrtpFecKey().getSrtpLifetime()
              << ", fecMkiValue=" << itCrypto->getSrtpFecKey().getSrtpMkiValue()
              << ", fecMkiLength=" << itCrypto->getSrtpFecKey().getSrtpMkiLength();
      }

      SdpMediaLine::SdpCrypto::GenericSessionParamList::const_iterator itSessParam = itCrypto->getGenericSessionParams().begin();
      for(;itSessParam!=itCrypto->getGenericSessionParams().end(); itSessParam++)
      {
         strm << std::endl << "        sessParam=" << *itSessParam;
      }
      strm << std::endl;
   }

   strm << "FingerPrint: type=" << SdpMediaLine::SdpFingerPrintHashFuncTypeString[sdpMediaLine.mFingerPrintHashFunction] 
        << ", '" << sdpMediaLine.mFingerPrint << "'" << std::endl
        << "KeyManagement: type=" << SdpMediaLine::SdpKeyManagementProtocolTypeString[sdpMediaLine.mKeyManagementProtocol]
        << ", '" << sdpMediaLine.mKeyManagementData << "'" << std::endl;

   SdpMediaLine::SdpPreConditionList::const_iterator itCurrentStatus = sdpMediaLine.mPreConditionCurrentStatus.begin();
   for(;itCurrentStatus != sdpMediaLine.mPreConditionCurrentStatus.end(); itCurrentStatus++)
   {
      strm << "PreConditionCurrentStatus: type=" << SdpMediaLine::SdpPreConditionTypeString[itCurrentStatus->getType()]
           << ", status=" << SdpMediaLine::SdpPreConditionStatusTypeString[itCurrentStatus->getStatus()]
           << ", direction=" << SdpMediaLine::SdpPreConditionDirectionTypeString[itCurrentStatus->getDirection()] << std::endl;
   }

   SdpMediaLine::SdpPreConditionList::const_iterator itConfirmStatus = sdpMediaLine.mPreConditionConfirmStatus.begin();
   for(;itConfirmStatus != sdpMediaLine.mPreConditionConfirmStatus.end(); itConfirmStatus++)
   {
      strm << "PreConditionConfirmStatus: type=" << SdpMediaLine::SdpPreConditionTypeString[itConfirmStatus->getType()]
           << ", status=" << SdpMediaLine::SdpPreConditionStatusTypeString[itConfirmStatus->getStatus()]
           << ", direction=" << SdpMediaLine::SdpPreConditionDirectionTypeString[itConfirmStatus->getDirection()] << std::endl;
   }

   SdpMediaLine::SdpPreConditionDesiredStatusList::const_iterator itDesiredStatus = sdpMediaLine.mPreConditionDesiredStatus.begin();
   for(;itDesiredStatus != sdpMediaLine.mPreConditionDesiredStatus.end(); itDesiredStatus++)
   {
      strm << "PreConditionDesiredStatus: type=" << SdpMediaLine::SdpPreConditionTypeString[itDesiredStatus->getType()]
           << ", strength=" << SdpMediaLine::SdpPreConditionStrengthTypeString[itDesiredStatus->getStrength()]
           << ", status=" << SdpMediaLine::SdpPreConditionStatusTypeString[itDesiredStatus->getStatus()]
           << ", direction=" << SdpMediaLine::SdpPreConditionDirectionTypeString[itDesiredStatus->getDirection()] << std::endl;
   }

   strm << "MaximumPacketRate: " << sdpMediaLine.mMaximumPacketRate << std::endl
        << "Label: '" << sdpMediaLine.mLabel << "'" << std::endl
        << "IdentificationTag: '" << sdpMediaLine.mIdentificationTag << "'" << std::endl
        << "IceUserFrag: '" << sdpMediaLine.mIceUserFrag << "'" << std::endl
        << "IcePassword: '" << sdpMediaLine.mIcePassword << "'" << std::endl;

   SdpMediaLine::SdpRemoteCandidateList::const_iterator itRemoteCandidate = sdpMediaLine.mRemoteCandidates.begin();
   for(;itRemoteCandidate != sdpMediaLine.mRemoteCandidates.end(); itRemoteCandidate++)
   {
      strm << "Remote Candidate: componentId=" << itRemoteCandidate->getComponentId()
           << ", addr=" << itRemoteCandidate->getConnectionAddress() 
           << ", port=" << itRemoteCandidate->getPort() << std::endl;
   }

   strm << "IceSupported: " << sdpMediaLine.isIceSupported() << std::endl;

   SdpMediaLine::SdpCandidateList::const_iterator itCandidate = sdpMediaLine.mCandidates.begin();
   for(;itCandidate != sdpMediaLine.mCandidates.end(); itCandidate++)
   {
      strm << *itCandidate;
   }

   SdpMediaLine::SdpCandidatePairList::const_iterator itCandidatePair = sdpMediaLine.mCandidatePairs.begin();
   for(;itCandidatePair != sdpMediaLine.mCandidatePairs.end(); itCandidatePair++)
   {
      strm << *itCandidatePair;
   }

   SdpMediaLine::SdpMediaLineList::const_iterator itPotentialMediaLine = sdpMediaLine.mPotentialMediaViews.begin();
   for(;itPotentialMediaLine!=sdpMediaLine.mPotentialMediaViews.end();itPotentialMediaLine++)
   {
      strm << "PotentialMediaView:" << std::endl << *itPotentialMediaLine;
   }

   strm << "PotentialMediaViewString: '" << sdpMediaLine.mPotentialMediaViewString << "'" << std::endl;

   return strm;
}


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
