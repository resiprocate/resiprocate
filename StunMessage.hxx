#if !defined(STUNMESSAGE_HXX)
#define STUNMESSAGE_HXX 

#include <ostream>
#include <rutil/Compat.hxx>
#include <rutil/Data.hxx>
#include <asio.hpp>

#include "StunTuple.hxx"

#define STUN_MAX_UNKNOWN_ATTRIBUTES 8

namespace reTurn
{

typedef struct { UInt32 longpart[4]; }  UInt128;
typedef struct { UInt32 longpart[3]; }  UInt96;

#ifdef __cplusplus
bool operator<(const UInt128&, const UInt128&);
bool operator==(const UInt128&, const UInt128&);
#endif

class StunMessage
{
public:
   explicit StunMessage(const StunTuple& localTuple,
                        const StunTuple& remoteTuple,
                        char* buf, unsigned int bufLen,
                        asio::ip::udp::endpoint* alternatePortEndpoint = 0,    // Currently these are just here so that the Request handler can 
                        asio::ip::udp::endpoint* alternateIpEndpoint = 0,      // access them - could just pass to the request handler when it is 
                        asio::ip::udp::endpoint* alternateIpPortEndpoint = 0); // created, then using the localTuple of the StunMessage it would 
                                                                               // need to figure out the correct alternates
                                                                               // Note: These are used for RFC3489 backwards compatibility support
   explicit StunMessage();

   StunMessage(const StunMessage& message);
   virtual ~StunMessage();
   StunMessage& operator=(const StunMessage& rhs);

   bool isValid() { return mIsValid; }

   unsigned int stunEncodeMessage(char* buf, unsigned int bufLen);

   void setErrorCode(unsigned short errorCode, const char* reason);
   void setUsername(const char* username);
   void setPassword(const char* password);
   void setRealm(const char* realm);
   void setNonce(const char* nonce);
   void setServer(const char* server);
   void setTurnData(const char* data, unsigned int len);

   void createHeader(UInt16 stunclass, UInt16 method);  // Set info needed for a new stun message - set's tid as well
   void createUsernameAndPassword();  // Ensure mRemoteTuple is set first
   void generateShortTermPasswordForUsername(resip::Data& password);  // Ensure username is set first
   void getAddressFromUsername(asio::ip::address& address, unsigned int& port);
   void calculateHmacKey(resip::Data& hmacKey, const resip::Data& longtermAuthenticationPassword);
   bool checkMessageIntegrity(const resip::Data& hmacKey);
   bool checkFingerprint();


   /// define stun address families
   const static UInt8  IPv4Family = 0x01;
   const static UInt8  IPv6Family = 0x02;

   const static UInt8 PortPropsNone     = 0;
   const static UInt8 PortPropsOdd      = 1;
   const static UInt8 PortPropsEven     = 2;
   const static UInt8 PortPropsEvenPair = 3;

   const static UInt32 RequestedTransportUdp = 0;
   const static UInt32 RequestedTransportTcp = 1;

   // define  flags  
   const static UInt32 ChangeIpFlag   = 0x04;
   const static UInt32 ChangePortFlag = 0x02;


   // Message Type - from RFC3489-bis-06
   //
   //        0                   1                   2                   3
   //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   //       |0 0|     STUN Message Type     |         Message Length        |
   //       |   |M|M|M|M|M|C|M|M|M|C|M|M|M|M|                               |
   //       |   |1|1|9|8|7|1|6|5|4|0|3|2|1|0|                               |
   //       |   |1|0| | | | | | | | | | | | |                               |
   //       |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
   //
   // M11 through M0 represent a 12-bit encoding of the method
   // C1 through C0 represent a 2 bit encoding of the class.
   // 2-bit Class Values are: 00=Request, 01=Indicaiton, 
   //                         10=Success Response, 11=Error Response
   //

   const static UInt16 StunClassRequest            = 0x0000;
   const static UInt16 StunClassIndication         = 0x0010;
   const static UInt16 StunClassSuccessResponse    = 0x0100;
   const static UInt16 StunClassErrorResponse      = 0x0110; 

   // define types for a stun message 
   // RFC3489-bis-06
   const static UInt16 BindRequest                  = 0x001;
   const static UInt16 SharedSecretRequest          = 0x002;

   // define types for a turn message - per behave-turn-03
   const static UInt16 TurnAllocateRequest         = 0x003;
   const static UInt16 TurnSetActiveDestinationRequest = 0x004;
   const static UInt16 TurnConnectRequest          = 0x005;
   // define types for a turn indication - per behave-turn-03
   const static UInt16 TurnSendInd                 = 0x001;
   const static UInt16 TurnDataInd                 = 0x002;
   const static UInt16 TurnConnectStatusInd        = 0x003;

   // define  stun attribute
   // RFC3489-bis-06
   const static UInt16 MappedAddress    = 0x0001;
   const static UInt16 ResponseAddress  = 0x0002;  // deprecated by RFC3489-bis-06
   const static UInt16 ChangeRequest    = 0x0003;  // deprecated by RFC3489-bis-06
   const static UInt16 SourceAddress    = 0x0004;  // deprecated by RFC3489-bis-06
   const static UInt16 ChangedAddress   = 0x0005;  // deprecated by RFC3489-bis-06
   const static UInt16 Username         = 0x0006;  
   const static UInt16 Password         = 0x0007;
   const static UInt16 MessageIntegrity = 0x0008;
   const static UInt16 ErrorCode        = 0x0009;
   const static UInt16 UnknownAttribute = 0x000A;
   const static UInt16 ReflectedFrom    = 0x000B;  // deprecated by RFC3489-bis-06
   const static UInt16 Realm            = 0x0014;
   const static UInt16 Nonce            = 0x0015;
   const static UInt16 XorMappedAddress = 0x0020;
   const static UInt16 XorMappedAddress_old = 0x8020; // deprecated
   const static UInt16 Fingerprint      = 0x0021;  
   const static UInt16 Server           = 0x8022;
   const static UInt16 AlternateServer  = 0x8023;
   const static UInt16 RefreshInterval  = 0x8024;
   const static UInt16 SecondaryAddress = 0x8050;  // Non standard extention

   // TURN specific message attributes - from behave-turn-03
   const static UInt16 TurnLifetime     = 0x000d;
   const static UInt16 TurnAlternateServer = 0x000e; // deprecated
   const static UInt16 TurnMagicCookie  = 0x000f;    // deprecated
   const static UInt16 TurnBandwidth    = 0x0010;
   const static UInt16 TurnDestinationAddress = 0x0011; // deprecated
   const static UInt16 TurnRemoteAddress = 0x0012; 
   const static UInt16 TurnData         = 0x0013;
   const static UInt16 TurnRelayAddress = 0x0016;
   const static UInt16 TurnRequestedPortProps = 0x0018;
   const static UInt16 TurnRequestedTransport = 0x0019;
   //const static UInt16 TurnTimerVal     = 0x0021; 
   const static UInt16 TurnRequestedIp  = 0x0022;
   const static UInt16 TurnConnectStat  = 0x0023;

   const static UInt32 StunMagicCookie  = 0x2112A442;
   typedef struct 
   {
      UInt32 magicCookie;
      UInt96 tid;
   } Id;

   typedef struct 
   {
      UInt16 msgType;
      UInt16 msgLength;
      union
      {
         UInt128 magicCookieAndTid;
         Id id;
      };
   } StunMsgHdr;

   typedef struct
   {
      UInt16 type;
      UInt16 length;
   } StunAtrHdr;

   typedef struct
   {
      UInt8 family;
      UInt16 port;
      union
      {
         UInt32 ipv4;
         UInt128 ipv6;
      } addr;
   } StunAtrAddress;

   typedef struct
   {
      UInt8 errorClass;
      UInt8 number;
      resip::Data* reason;
   } StunAtrError;

   typedef struct
   {
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      UInt16 numAttributes;
   } StunAtrUnknown;

   typedef struct
   {
      char hash[20];
   } StunAtrIntegrity;

   typedef struct
   {
      UInt8 props;
      UInt16 port;
   } TurnAtrRequestedPortProps;

   enum StunHmacStatus
   {
      HmacUnkown=0,
      HmacOK,
      HmacBadUserName,
      HmacUnkownUserName,
      HmacFailed
   };

   // !slg! TODO make private and provide accessors
   UInt16 mClass;    // Request, Response, Indication
   UInt16 mMethod;   // Binding Request, Shared Secret Request, Allocation Request, etc.
   bool mHasMagicCookie;  // Set to true if stun magic cookie is in message header
   StunTuple mLocalTuple;  // Local address and port that received stun message
   StunTuple mRemoteTuple; // Remote address and port that send stun message
   asio::ip::udp::endpoint* mAlternatePortEndpoint;   // for RFC3489 backwards compatibility - NAT Type detection
   asio::ip::udp::endpoint* mAlternateIpEndpoint;     // for RFC3489 backwards compatibility - NAT Type detection
   asio::ip::udp::endpoint* mAlternateIpPortEndpoint; // for RFC3489 backwards compatibility - NAT Type detection
   resip::Data mBuffer;
   resip::Data mHmacKey;

   StunMsgHdr mHeader;

   bool mHasMappedAddress;
   StunAtrAddress  mMappedAddress;

   bool mHasResponseAddress;
   StunAtrAddress  mResponseAddress;

   bool mHasChangeRequest;
   UInt32 mChangeRequest;

   bool mHasSourceAddress;
   StunAtrAddress mSourceAddress;

   bool mHasChangedAddress;
   StunAtrAddress mChangedAddress;

   bool mHasUsername;
   resip::Data* mUsername;

   bool mHasPassword;
   resip::Data* mPassword;

   bool mHasMessageIntegrity;
   StunAtrIntegrity mMessageIntegrity;

   bool mHasErrorCode;
   StunAtrError mErrorCode;

   bool mHasUnknownAttributes;
   StunAtrUnknown mUnknownAttributes;

   bool mHasReflectedFrom;
   StunAtrAddress mReflectedFrom;

   bool mHasRealm;
   resip::Data* mRealm;

   bool mHasNonce;
   resip::Data* mNonce;

   bool mHasXorMappedAddress;
   StunAtrAddress  mXorMappedAddress;

   bool mHasFingerprint;
   UInt32 mFingerprint;

   bool mHasServer;
   resip::Data* mServer;

   bool mHasAlternateServer;
   StunAtrAddress mAlternateServer;

   bool mHasRefreshInterval;
   UInt32 mRefreshInterval;

   bool mHasSecondaryAddress;
   StunAtrAddress mSecondaryAddress;

   // Turn Attributes
   bool mHasTurnLifetime;
   UInt32 mTurnLifetime;

   bool mHasTurnAlternateServer;
   StunAtrAddress mTurnAlternateServer;

   bool mHasTurnMagicCookie;
   UInt32 mTurnMagicCookie;

   bool mHasTurnBandwidth;
   UInt32 mTurnBandwidth;

   bool mHasTurnDestinationAddress;
   StunAtrAddress mTurnDestinationAddress;

   bool mHasTurnRemoteAddress;
   StunAtrAddress mTurnRemoteAddress;

   bool mHasTurnData;
   resip::Data* mTurnData;

   bool mHasTurnRelayAddress;
   StunAtrAddress mTurnRelayAddress;

   bool mHasTurnRequestedPortProps;
   TurnAtrRequestedPortProps mTurnRequestedPortProps;

   bool mHasTurnRequestedTransport;
   UInt32 mTurnRequestedTransport;

   bool mHasTurnRequestedIp;
   StunAtrAddress mTurnRequestedIp;

   bool mHasTurnConnectStat;
   UInt32 mTurnConnectStat;

   void applyXorToAddress(const StunAtrAddress& in, StunAtrAddress& out);  // ensure tid is set first

protected:

private:
   void init();

   bool stunParseAtrAddress( char* body, unsigned int hdrLen,  StunAtrAddress& result );
   bool stunParseAtrUInt32( char* body, unsigned int hdrLen,  UInt32& result );
   bool stunParseAtrError( char* body, unsigned int hdrLen,  StunAtrError& result );
   bool stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown& result );
   bool stunParseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity& result );
   bool stunParseAtrRequestedPortProps( char* body, unsigned int hdrLen,  TurnAtrRequestedPortProps& result );

   bool stunParseMessage( char* buf, unsigned int bufLen);

   char* encode16(char* buf, UInt16 data);
   char* encode32(char* buf, UInt32 data);
   char* encode(char* buf, const char* data, unsigned int length);
   char* encodeTurnData(char *ptr, const resip::Data* td);
   char* encodeAtrUInt32(char* ptr, UInt16 type, UInt32 value);
   char* encodeAtrAddress(char* ptr, UInt16 type, const StunAtrAddress& atr);
   char* encodeAtrError(char* ptr, const StunAtrError& atr);
   char* encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr);
   char* encodeAtrString(char* ptr, UInt16 type, const resip::Data* atr);
   char* encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr);
   char* encodeAtrRequestedPortProps(char* ptr, const TurnAtrRequestedPortProps& atr);
   void computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey);

   bool mIsValid;
};

std::ostream& operator<< ( std::ostream& strm, const StunMessage::StunAtrAddress& addr);
std::ostream& operator<< ( std::ostream& strm, const UInt128& );
std::ostream& operator<< ( std::ostream& strm, const StunMessage::StunMsgHdr& );

}

#endif


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

