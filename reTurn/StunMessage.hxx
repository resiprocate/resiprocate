#if !defined(STUNMESSAGE_HXX)
#define STUNMESSAGE_HXX 

#include <ostream>
#include <rutil/compat.hxx>
#include <rutil/Data.hxx>

#include "StunTuple.hxx"

#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define TURN_MAX_XOR_PEER_ADDR      8

namespace reTurn
{

typedef struct { uint32_t longpart[4]; }  UInt128;
typedef struct { uint32_t longpart[3]; }  UInt96;

#ifdef __cplusplus
bool operator<(const UInt128&, const UInt128&);
bool operator==(const UInt128&, const UInt128&);
#endif

class StunMessage
{
public:
   explicit StunMessage(const StunTuple& localTuple,
                        const StunTuple& remoteTuple,
                        char* buf, unsigned int bufLen);

   explicit StunMessage();

   StunMessage(const StunMessage& message);
   virtual ~StunMessage();
   StunMessage& operator=(const StunMessage& rhs);

   bool isValid() { return mIsValid; }
   bool hasMagicCookie();

   unsigned int stunEncodeMessage(char* buf, unsigned int bufLen);
   unsigned int stunEncodeFramedMessage(char* buf, unsigned int bufLen);  // Used for TURN-05 framing only

   void setErrorCode(unsigned short errorCode, const char* reason);
   void setUsername(const char* username);
   void setPassword(const char* password);
   void setRealm(const char* realm);
   void setNonce(const char* nonce);
   void setSoftware(const char* software);
   void setTurnData(const char* data, unsigned int len);

   // ICE-specific attributes
   void setIcePriority(uint32_t priority);
   void setIceUseCandidate();
   void setIceControlled();
   void setIceControlling();

   void createHeader(uint16_t stunclass, uint16_t method);  // Set info needed for a new stun message - set's tid as well
   void createUsernameAndPassword();  // Ensure mRemoteTuple is set first
   void generateShortTermPasswordForUsername(resip::Data& password);  // Ensure username is set first
   void getTupleFromUsername(StunTuple& tuple);   // note: does not set transport type
   void calculateHmacKey(resip::Data& hmacKey, const resip::Data& longtermAuthenticationPassword);
   void calculateHmacKeyForHa1(resip::Data& hmacKey, const resip::Data& ha1);
   void calculateHmacKey(resip::Data& hmacKey, const resip::Data& username, const resip::Data& realm, const resip::Data& longtermAuthenticationPassword);
   bool checkMessageIntegrity(const resip::Data& hmacKey);
   bool checkFingerprint();

   /// define stun address families
   const static uint8_t  IPv4Family = 0x01;
   const static uint8_t  IPv6Family = 0x02;

   const static uint8_t PropsPortEven = 0x00;
   const static uint8_t PropsPortPair = 0x80;  
   const static uint8_t PropsNone     = 0xFF;

   // The following are codepoints used in the requested transport header, they
   // are the same values used in the IPv4 and IPv6 headers
   const static uint32_t RequestedTransportUdp = 17;
   const static uint32_t RequestedTransportTcp = 6;

   // define  flags  
   const static uint32_t ChangeIpFlag   = 0x04;
   const static uint32_t ChangePortFlag = 0x02;


   // Message Type - from RFC5389
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

   const static uint16_t StunClassRequest            = 0x0000;
   const static uint16_t StunClassIndication         = 0x0010;
   const static uint16_t StunClassSuccessResponse    = 0x0100;
   const static uint16_t StunClassErrorResponse      = 0x0110; 

   // define types for a stun message - RFC5389
   const static uint16_t BindMethod                  = 0x001;
   const static uint16_t SharedSecretMethod          = 0x002;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)

   // define types for a turn message - per RFC5766
   const static uint16_t TurnAllocateMethod          = 0x003;
   const static uint16_t TurnRefreshMethod           = 0x004;
   const static uint16_t TurnSendMethod              = 0x006;  // indication only
   const static uint16_t TurnDataMethod              = 0x007;  // indication only
   const static uint16_t TurnCreatePermissionMethod  = 0x008;
   const static uint16_t TurnChannelBindMethod       = 0x009;

   // define  stun attribute - RFC5389
   // Comprehension required attributes
   const static uint16_t MappedAddress    = 0x0001;
   const static uint16_t ResponseAddress  = 0x0002;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t ChangeRequest    = 0x0003;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t SourceAddress    = 0x0004;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t ChangedAddress   = 0x0005;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t Username         = 0x0006;  
   const static uint16_t Password         = 0x0007;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t MessageIntegrity = 0x0008;
   const static uint16_t ErrorCode        = 0x0009;
   const static uint16_t UnknownAttribute = 0x000A;
   const static uint16_t ReflectedFrom    = 0x000B;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static uint16_t Realm            = 0x0014;
   const static uint16_t Nonce            = 0x0015;
   const static uint16_t XorMappedAddress = 0x0020;
   const static uint16_t XorMappedAddress_old = 0x8020; // deprecated
   // Comprehension Optional Attributes
   const static uint16_t Software         = 0x8022;
   const static uint16_t AlternateServer  = 0x8023;
   const static uint16_t Fingerprint      = 0x8028;  
   const static uint16_t SecondaryAddress = 0x8050;  // Non standard extension

   // TURN specific message attributes - from behave-turn-12
   const static uint16_t TurnChannelNumber      = 0x000C;
   const static uint16_t TurnLifetime           = 0x000D;
   const static uint16_t TurnBandwidth          = 0x0010; // reserved (removed from latest draft)
   const static uint16_t TurnXorPeerAddress     = 0x0012;
   const static uint16_t TurnData               = 0x0013;
   const static uint16_t TurnXorRelayedAddress  = 0x0016;
   const static uint16_t TurnEvenPort           = 0x0018;
   const static uint16_t TurnRequestedTransport = 0x0019;
   const static uint16_t TurnDontFragment       = 0x001a;
   //const static UInt16 TurnTimerVal         = 0x0021; // reserved (removed from latest draft)
   const static uint16_t TurnReservationToken   = 0x0022;
   const static uint16_t TurnConnectStat        = 0x0023; // tcp allocations
   const static uint16_t TurnRequestedAddressFamily = 0x0017; // RFC 6156

   // ICE specific message attributes - from draft-ietf-mmusic-ice-19
   const static uint16_t IcePriority            = 0x0024;
   const static uint16_t IceUseCandidate        = 0x0025;
   const static uint16_t IceControlled          = 0x8029;
   const static uint16_t IceControlling         = 0x802A;

   const static uint32_t StunMagicCookie  = 0x2112A442;  
   typedef struct 
   {
      uint32_t magicCookie;
      UInt96 tid;
   } Id;

   typedef struct 
   {
      uint16_t msgType;
      uint16_t msgLength;
      union
      {
         UInt128 magicCookieAndTid;
         Id id;
      };
   } StunMsgHdr;

   typedef struct
   {
      uint16_t type;
      uint16_t length;
   } StunAtrHdr;

   typedef struct
   {
      uint8_t family;
      uint16_t port;
      union
      {
         uint32_t ipv4;  // in host byte order
         UInt128 ipv6; // in network byte order
      } addr;
   } StunAtrAddress;

   typedef struct
   {
      uint8_t errorClass;
      uint8_t number;
      resip::Data* reason;
   } StunAtrError;

   typedef struct
   {
      uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      uint16_t numAttributes;
   } StunAtrUnknown;

   typedef struct
   {
      char hash[20];
   } StunAtrIntegrity;

   typedef struct
   {
      uint8_t propType;
   } TurnAtrEvenPort;

   enum StunHmacStatus
   {
      HmacUnknown=0,
      HmacOK,
      HmacBadUserName,
      HmacUnknownUserName,
      HmacFailed
   };

   // !slg! TODO make private and provide accessors
   uint16_t mClass;    // Request, Response, Indication
   uint16_t mMethod;   // Binding Request, Shared Secret Request, Allocation Request, etc.
   bool mHasMagicCookie;  // Set to true if stun magic cookie is in message header
   StunTuple mLocalTuple;  // Local address and port that received the stun message
   StunTuple mRemoteTuple; // Remote address and port that sent the stun message
   resip::Data mBuffer;
   resip::Data mHmacKey;

   uint16_t mMessageIntegrityMsgLength;

   StunMsgHdr mHeader;

   bool mHasMappedAddress;
   StunAtrAddress  mMappedAddress;

   bool mHasResponseAddress;
   StunAtrAddress  mResponseAddress;

   bool mHasChangeRequest;
   uint32_t mChangeRequest;

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
   uint32_t mFingerprint;

   bool mHasSoftware;
   resip::Data* mSoftware;

   bool mHasAlternateServer;
   StunAtrAddress mAlternateServer;

   bool mHasSecondaryAddress;
   StunAtrAddress mSecondaryAddress;

   // Turn Attributes
   bool mHasTurnChannelNumber;
   uint16_t mTurnChannelNumber;

   bool mHasTurnLifetime;
   uint32_t mTurnLifetime;

   bool mHasTurnBandwidth;
   uint32_t mTurnBandwidth;

   int mCntTurnXorPeerAddress;
   StunAtrAddress mTurnXorPeerAddress[TURN_MAX_XOR_PEER_ADDR];

   bool mHasTurnData;
   resip::Data* mTurnData;

   bool mHasTurnXorRelayedAddress;
   StunAtrAddress mTurnXorRelayedAddress;

   bool mHasTurnEvenPort;
   TurnAtrEvenPort mTurnEvenPort;

   bool mHasTurnRequestedTransport;
   uint8_t mTurnRequestedTransport;

   bool mHasTurnDontFragment;
   // No attribute data

   bool mHasTurnReservationToken;
   uint64_t mTurnReservationToken;

   bool mHasTurnConnectStat;
   uint32_t mTurnConnectStat;

   bool mHasTurnRequestedAddressFamily;
   uint16_t mTurnRequestedAddressFamily;

   bool mHasIcePriority;
   uint32_t mIcePriority;

   bool mHasIceUseCandidate;

   bool mHasIceControlled;
   uint64_t mIceControlledTieBreaker;

   bool mHasIceControlling;
   uint64_t mIceControllingTieBreaker;

   StunAtrUnknown mUnknownRequiredAttributes;

   // Utility APIs
   void applyXorToAddress(const StunAtrAddress& in, StunAtrAddress& out);  // ensure tid is set first
   static void setStunAtrAddressFromTuple(StunAtrAddress& address, const StunTuple& tuple);
   static void setTupleFromStunAtrAddress(StunTuple& tuple, const StunAtrAddress& address);  // Note:  does not set transport type

protected:

private:
   void init();

   bool stunParseAtrXorAddress( char* body, unsigned int hdrLen, StunAtrAddress& result );
   bool stunParseAtrAddress( char* body, unsigned int hdrLen, StunAtrAddress& result );
   bool stunParseAtrUInt32( char* body, unsigned int hdrLen, uint32_t& result );
   bool stunParseAtrUInt64( char* body, unsigned int hdrLen, uint64_t& result );
   bool stunParseAtrError( char* body, unsigned int hdrLen, StunAtrError& result );
   bool stunParseAtrUnknown( char* body, unsigned int hdrLen, StunAtrUnknown& result );
   bool stunParseAtrIntegrity( char* body, unsigned int hdrLen, StunAtrIntegrity& result );
   bool stunParseAtrEvenPort( char* body, unsigned int hdrLen, TurnAtrEvenPort& result );

   bool stunParseMessage( char* buf, unsigned int bufLen);

   char* encode16(char* buf, uint16_t data);
   char* encode32(char* buf, uint32_t data);
   char* encode64(char* buf, const uint64_t data);
   char* encode(char* buf, const char* data, unsigned int length);
   char* encodeTurnData(char *ptr, const resip::Data* td);
   char* encodeAtrUInt32(char* ptr, uint16_t type, uint32_t value);
   char* encodeAtrUInt64(char* ptr, uint16_t type, uint64_t value);
   char* encodeAtrXorAddress(char* ptr, uint16_t type, const StunAtrAddress& atr);
   char* encodeAtrAddress(char* ptr, uint16_t type, const StunAtrAddress& atr);
   char* encodeAtrError(char* ptr, const StunAtrError& atr);
   char* encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr);
   char* encodeAtrString(char* ptr, uint16_t type, const resip::Data* atr, uint16_t maxBytes);
   char* encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr);
   char* encodeAtrEvenPort(char* ptr, const TurnAtrEvenPort& atr);
   void computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey);

   bool mIsValid;
};

EncodeStream& operator<< ( EncodeStream& strm, const StunMessage::StunAtrAddress& addr);
EncodeStream& operator<< ( EncodeStream& strm, const UInt128& );
EncodeStream& operator<< ( EncodeStream& strm, const StunMessage::StunMsgHdr& );

}

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
