#if !defined(STUNMESSAGE_HXX)
#define STUNMESSAGE_HXX 

#include <ostream>
#include <rutil/compat.hxx>
#include <rutil/Data.hxx>
#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif

#include "StunTuple.hxx"

#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define TURN_MAX_XOR_PEER_ADDR      8

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
   void setIcePriority(UInt32 priority);
   void setIceUseCandidate();
   void setIceControlled();
   void setIceControlling();

   void createHeader(UInt16 stunclass, UInt16 method);  // Set info needed for a new stun message - set's tid as well
   void createUsernameAndPassword();  // Ensure mRemoteTuple is set first
   void generateShortTermPasswordForUsername(resip::Data& password);  // Ensure username is set first
   void getTupleFromUsername(StunTuple& tuple);   // note: does not set transport type
   void calculateHmacKey(resip::Data& hmacKey, const resip::Data& longtermAuthenticationPassword);
   void calculateHmacKeyForHa1(resip::Data& hmacKey, const resip::Data& ha1);
   void calculateHmacKey(resip::Data& hmacKey, const resip::Data& username, const resip::Data& realm, const resip::Data& longtermAuthenticationPassword);
   bool checkMessageIntegrity(const resip::Data& hmacKey);
   bool checkFingerprint();

   /// define stun address families
   const static UInt8  IPv4Family = 0x01;
   const static UInt8  IPv6Family = 0x02;

   const static UInt8 PropsPortEven = 0x00;
   const static UInt8 PropsPortPair = 0x80;  
   const static UInt8 PropsNone     = 0xFF;

   // The following are codepoints used in the requested transport header, they
   // are the same values used in the IPv4 and IPv6 headers
   const static UInt32 RequestedTransportUdp = 17;
   const static UInt32 RequestedTransportTcp = 6;

   // define  flags  
   const static UInt32 ChangeIpFlag   = 0x04;
   const static UInt32 ChangePortFlag = 0x02;


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

   const static UInt16 StunClassRequest            = 0x0000;
   const static UInt16 StunClassIndication         = 0x0010;
   const static UInt16 StunClassSuccessResponse    = 0x0100;
   const static UInt16 StunClassErrorResponse      = 0x0110; 

   // define types for a stun message - RFC5389
   const static UInt16 BindMethod                  = 0x001;
   const static UInt16 SharedSecretMethod          = 0x002;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)

   // define types for a turn message - per RFC5766
   const static UInt16 TurnAllocateMethod          = 0x003;
   const static UInt16 TurnRefreshMethod           = 0x004;
   const static UInt16 TurnSendMethod              = 0x006;  // indication only
   const static UInt16 TurnDataMethod              = 0x007;  // indication only
   const static UInt16 TurnCreatePermissionMethod  = 0x008;
   const static UInt16 TurnChannelBindMethod       = 0x009;

   // define  stun attribute - RFC5389
   // Comprehension required attributes
   const static UInt16 MappedAddress    = 0x0001;
   const static UInt16 ResponseAddress  = 0x0002;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 ChangeRequest    = 0x0003;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 SourceAddress    = 0x0004;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 ChangedAddress   = 0x0005;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 Username         = 0x0006;  
   const static UInt16 Password         = 0x0007;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 MessageIntegrity = 0x0008;
   const static UInt16 ErrorCode        = 0x0009;
   const static UInt16 UnknownAttribute = 0x000A;
   const static UInt16 ReflectedFrom    = 0x000B;  // deprecated by RFC5389 (used for backwards compatibility to RFC3489 only)
   const static UInt16 Realm            = 0x0014;
   const static UInt16 Nonce            = 0x0015;
   const static UInt16 XorMappedAddress = 0x0020;
   const static UInt16 XorMappedAddress_old = 0x8020; // deprecated
   // Comprehension Optional Attributes
   const static UInt16 Software         = 0x8022;
   const static UInt16 AlternateServer  = 0x8023;
   const static UInt16 Fingerprint      = 0x8028;  
   const static UInt16 SecondaryAddress = 0x8050;  // Non standard extension

   // TURN specific message attributes - from behave-turn-12
   const static UInt16 TurnChannelNumber      = 0x000C;
   const static UInt16 TurnLifetime           = 0x000D;
   const static UInt16 TurnBandwidth          = 0x0010; // reserved (removed from latest draft)
   const static UInt16 TurnXorPeerAddress     = 0x0012;
   const static UInt16 TurnData               = 0x0013;
   const static UInt16 TurnXorRelayedAddress  = 0x0016;
   const static UInt16 TurnEvenPort           = 0x0018;
   const static UInt16 TurnRequestedTransport = 0x0019;
   const static UInt16 TurnDontFragment       = 0x001a;
   //const static UInt16 TurnTimerVal         = 0x0021; // reserved (removed from latest draft)
   const static UInt16 TurnReservationToken   = 0x0022;
   const static UInt16 TurnConnectStat        = 0x0023; // tcp allocations
   const static UInt16 TurnRequestedAddressFamily = 0x0017; // RFC 6156

   // ICE specific message attributes - from draft-ietf-mmusic-ice-19
   const static UInt16 IcePriority            = 0x0024;
   const static UInt16 IceUseCandidate        = 0x0025;
   const static UInt16 IceControlled          = 0x8029;
   const static UInt16 IceControlling         = 0x802A;

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
         UInt32 ipv4;  // in host byte order
         UInt128 ipv6; // in network byte order
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
      UInt8 propType;
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
   UInt16 mClass;    // Request, Response, Indication
   UInt16 mMethod;   // Binding Request, Shared Secret Request, Allocation Request, etc.
   bool mHasMagicCookie;  // Set to true if stun magic cookie is in message header
   StunTuple mLocalTuple;  // Local address and port that received the stun message
   StunTuple mRemoteTuple; // Remote address and port that sent the stun message
   resip::Data mBuffer;
   resip::Data mHmacKey;

   UInt16 mMessageIntegrityMsgLength;

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

   bool mHasSoftware;
   resip::Data* mSoftware;

   bool mHasAlternateServer;
   StunAtrAddress mAlternateServer;

   bool mHasSecondaryAddress;
   StunAtrAddress mSecondaryAddress;

   // Turn Attributes
   bool mHasTurnChannelNumber;
   UInt16 mTurnChannelNumber;

   bool mHasTurnLifetime;
   UInt32 mTurnLifetime;

   bool mHasTurnBandwidth;
   UInt32 mTurnBandwidth;

   int mCntTurnXorPeerAddress;
   StunAtrAddress mTurnXorPeerAddress[TURN_MAX_XOR_PEER_ADDR];

   bool mHasTurnData;
   resip::Data* mTurnData;

   bool mHasTurnXorRelayedAddress;
   StunAtrAddress mTurnXorRelayedAddress;

   bool mHasTurnEvenPort;
   TurnAtrEvenPort mTurnEvenPort;

   bool mHasTurnRequestedTransport;
   UInt8 mTurnRequestedTransport;

   bool mHasTurnDontFragment;
   // No attribute data

   bool mHasTurnReservationToken;
   UInt64 mTurnReservationToken;

   bool mHasTurnConnectStat;
   UInt32 mTurnConnectStat;

   bool mHasTurnRequestedAddressFamily;
   UInt16 mTurnRequestedAddressFamily;

   bool mHasIcePriority;
   UInt32 mIcePriority;

   bool mHasIceUseCandidate;

   bool mHasIceControlled;
   UInt64 mIceControlledTieBreaker;

   bool mHasIceControlling;
   UInt64 mIceControllingTieBreaker;

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
   bool stunParseAtrUInt32( char* body, unsigned int hdrLen, UInt32& result );
   bool stunParseAtrUInt64( char* body, unsigned int hdrLen, UInt64& result );
   bool stunParseAtrError( char* body, unsigned int hdrLen, StunAtrError& result );
   bool stunParseAtrUnknown( char* body, unsigned int hdrLen, StunAtrUnknown& result );
   bool stunParseAtrIntegrity( char* body, unsigned int hdrLen, StunAtrIntegrity& result );
   bool stunParseAtrEvenPort( char* body, unsigned int hdrLen, TurnAtrEvenPort& result );

   bool stunParseMessage( char* buf, unsigned int bufLen);

   char* encode16(char* buf, UInt16 data);
   char* encode32(char* buf, UInt32 data);
   char* encode64(char* buf, const UInt64 data);
   char* encode(char* buf, const char* data, unsigned int length);
   char* encodeTurnData(char *ptr, const resip::Data* td);
   char* encodeAtrUInt32(char* ptr, UInt16 type, UInt32 value);
   char* encodeAtrUInt64(char* ptr, UInt16 type, UInt64 value);
   char* encodeAtrXorAddress(char* ptr, UInt16 type, const StunAtrAddress& atr);
   char* encodeAtrAddress(char* ptr, UInt16 type, const StunAtrAddress& atr);
   char* encodeAtrError(char* ptr, const StunAtrError& atr);
   char* encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr);
   char* encodeAtrString(char* ptr, UInt16 type, const resip::Data* atr, UInt16 maxBytes);
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
