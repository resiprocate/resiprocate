#ifndef STUN_HXX
#define STUN_HXX

#include <iostream>
#include <time.h>
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"

// if you change this version, change in makefile too 
#define STUN_VERSION "0.96"

#define STUN_MAX_STRING 256
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define STUN_MAX_MESSAGE_SIZE 2048

#define STUN_PORT 3478

#ifndef RESIP_COMPAT_HXX
// define some basic types
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
#ifdef __APPLE__
typedef unsigned long   UInt32;
#else
typedef unsigned int   UInt32;
#endif
#if defined( WIN32 )
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long UInt64;
#endif
#endif

typedef struct { unsigned char octet[16]; }  UInt128;

#ifdef __cplusplus
bool operator<(const UInt128&, const UInt128&);
bool operator==(const UInt128&, const UInt128&);
#endif

/// define a structure to hold a stun address 
const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;

// define  flags  
const UInt32 ChangeIpFlag   = 0x04;
const UInt32 ChangePortFlag = 0x02;

// define  stun attribute
const UInt16 MappedAddress    = 0x0001;
const UInt16 ResponseAddress  = 0x0002;
const UInt16 ChangeRequest    = 0x0003;
const UInt16 SourceAddress    = 0x0004;
const UInt16 ChangedAddress   = 0x0005;
const UInt16 Username         = 0x0006;
const UInt16 Password         = 0x0007;
const UInt16 MessageIntegrity = 0x0008;
const UInt16 ErrorCode        = 0x0009;
const UInt16 UnknownAttribute = 0x000A;
const UInt16 ReflectedFrom    = 0x000B;
const UInt16 XorMappedAddress = 0x8020;
const UInt16 XorOnly          = 0x0021;
const UInt16 ServerName       = 0x8022;
const UInt16 SecondaryAddress = 0x8050; // Non standard extention

// !jf! TURN specific message attributes - from turn-08
const UInt16 TurnLifetime     = 0x000d;
const UInt16 TurnAlternateServer = 0x000e;
const UInt16 TurnMagicCookie = 0x000f;
const UInt16 TurnBandwidth = 0x0010;
const UInt16 TurnDestinationAddress = 0x0011;
const UInt16 TurnRemoteAddress = 0x0012;
const UInt16 TurnData = 0x0013;
const UInt16 TurnNonce = 0x0014;
const UInt16 TurnRealm = 0x0015;


// define types for a stun message 
const UInt16 BindRequestMsg               = 0x0001;
const UInt16 BindResponseMsg              = 0x0101;
const UInt16 BindErrorResponseMsg         = 0x0111;
const UInt16 SharedSecretRequestMsg       = 0x0002;
const UInt16 SharedSecretResponseMsg      = 0x0102;
const UInt16 SharedSecretErrorResponseMsg = 0x0112;

// define types for a turn message - per turn-08
const UInt16 TurnAllocateRequest = 0x0003;
const UInt16 TurnAllocateResponse = 0x0103;
const UInt16 TurnAllocateErrorResponse = 0x0113;
const UInt16 TurnSendRequest = 0x0004;
const UInt16 TurnSendResponse = 0x0104;
const UInt16 TurnSendErrorResponse = 0x0114;
const UInt16 TurnDataIndication = 0x0115;
const UInt16 TurnSetActiveDestinationRequest = 0x0006;
const UInt16 TurnSetActiveDestinationResponse = 0x0106;
const UInt16 TurnSetActiveDestinationErrorResponse = 0x0116;

typedef struct 
{
      UInt16 msgType;
      UInt16 msgLength;
      UInt128 id;
} StunMsgHdr;

#ifdef __cplusplus
bool operator<(const StunMsgHdr&, const StunMsgHdr&);
#endif

typedef struct
{
      UInt16 type;
      UInt16 length;
} StunAtrHdr;

typedef struct
{
      UInt16 port;
      UInt32 addr;
} StunAddress4;

typedef struct
{
      UInt8 pad;
      UInt8 family;
      StunAddress4 ipv4;
} StunAtrAddress4;

typedef struct
{
      UInt32 value;
} StunAtrChangeRequest;

typedef struct
{
      UInt16 pad; // all 0
      UInt8 errorClass;
      UInt8 number;
      char reason[STUN_MAX_STRING];
      UInt16 sizeReason;
} StunAtrError;

typedef struct
{
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      UInt16 numAttributes;
} StunAtrUnknown;

typedef struct
{
      char value[STUN_MAX_STRING];      
      UInt16 sizeValue;
} StunAtrString;

typedef struct
{
      char hash[20];
} StunAtrIntegrity;


enum StunHmacStatus
{
   HmacUnknown=0,
   HmacOK,
   HmacBadUserName,
   HmacUnknownUserName,
   HmacFailed
};


typedef struct
{
      StunMsgHdr msgHdr;
	
      bool hasMappedAddress;
      StunAtrAddress4  mappedAddress;
	
      bool hasResponseAddress;
      StunAtrAddress4  responseAddress;
	
      bool hasChangeRequest;
      StunAtrChangeRequest changeRequest;
	
      bool hasSourceAddress;
      StunAtrAddress4 sourceAddress;
	
      bool hasChangedAddress;
      StunAtrAddress4 changedAddress;
	
      bool hasUsername;
      StunAtrString username;
	
      bool hasPassword;
      StunAtrString password;
	
      bool hasMessageIntegrity;
      StunAtrIntegrity messageIntegrity;
	
      bool hasErrorCode;
      StunAtrError errorCode;
	
      bool hasUnknownAttributes;
      StunAtrUnknown unknownAttributes;
	
      bool hasReflectedFrom;
      StunAtrAddress4 reflectedFrom;

      bool hasXorMappedAddress;
      StunAtrAddress4  xorMappedAddress;
	
      bool xorOnly;

      bool hasServerName;
      StunAtrString serverName;
      
      bool hasSecondaryAddress;
      StunAtrAddress4 secondaryAddress;

      bool hasTurnLifetime;
      UInt32 turnLifetime;
      
      bool hasTurnAlternateServer;
      StunAtrAddress4 turnAlternateServer;
      
      bool hasTurnMagicCookie;
      UInt32 turnMagicCookie;
      
      bool hasTurnBandwidth;
      UInt32 turnBandwidth;
      
      bool hasTurnDestinationAddress;
      StunAtrAddress4 turnDestinationAddress;
      
      bool hasTurnRemoteAddress;
      StunAtrAddress4 turnRemoteAddress;
      
      bool hasTurnData;
      resip::Data* turnData;
      
      //bool hasTurnNonce;
      // turnNonce;
      
      //bool hasTurnRealm;
      // turnRealm;
} StunMessage; 


// Define enum with different types of NAT 
enum NatType
{
   StunTypeUnknown=0,
   StunTypeFailure,
   StunTypeOpen,
   StunTypeBlocked,

   StunTypeIndependentFilter,
   StunTypeDependentFilter,
   StunTypePortDependedFilter,
   StunTypeDependentMapping,

   //StunTypeConeNat,
   //StunTypeRestrictedNat,
   //StunTypePortRestrictedNat,
   //StunTypeSymNat,
   
   StunTypeFirewall
};


#define MAX_MEDIA_RELAYS 500
#define MAX_RTP_MSG_SIZE 1500
#define MEDIA_RELAY_TIMEOUT 3*60

typedef struct 
{
      int relayPort;       // media relay port
      int fd;              // media relay file descriptor
      StunAddress4 destination; // NAT IP:port
      time_t expireTime;      // if no activity after time, close the socket 
} StunMediaRelay;

typedef struct
{
      StunAddress4 myAddr;
      StunAddress4 altAddr;
      resip::Socket myFd;
      resip::Socket altPortFd;
      resip::Socket altIpFd;
      resip::Socket altIpPortFd;
      bool relay; // true if media relaying is to be done
      StunMediaRelay relays[MAX_MEDIA_RELAYS];
} StunServerInfo;

bool
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage& message, 
                  bool verbose );

void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString& username,
                    bool changePort, bool changeIp, unsigned int id=0 );

unsigned int
stunEncodeMessage( const StunMessage& message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString& password,
                   bool verbose);

void
stunCreateUserName(const StunAddress4& addr, StunAtrString* username);

void 
stunGetUserNameAndPassword(  const StunAddress4& dest, 
                             StunAtrString* username,
                             StunAtrString* password);

void
stunCreatePassword(const StunAtrString& username, StunAtrString* password);

int 
stunRand();

UInt64
stunGetSystemTimeSecs();

/// find the IP address of a the specified stun server - return false is fails parse 
bool  
stunParseServerName( char* serverName, StunAddress4& stunServerAddr);

bool 
stunParseHostName( char* peerName,
                   UInt32& ip,
                   UInt16& portVal,
                   UInt16 defaultPort );

/// return true if all is OK
/// Create a media relay and do the STERN thing if startMediaPort is non-zero
bool
stunInitServer(StunServerInfo& info, 
               const StunAddress4& myAddr, 
               const StunAddress4& altAddr,
               int startMediaPort,
               bool verbose);

void
stunStopServer(StunServerInfo& info);

/// return true if all is OK 
bool
stunServerProcess(StunServerInfo& info, bool verbose);

/// returns number of address found - take array or addres 
int 
stunFindLocalInterfaces(UInt32* addresses, int maxSize );

bool 
stunTest( StunAddress4& dest, int testNum, bool verbose, StunAddress4* srcAddr=0, unsigned long timeoutMs=5000 );

NatType
stunNatType( StunAddress4& dest, bool verbose, 
             bool* preservePort=0, // if set, is return for if NAT preservers ports or not
             bool* hairpin=0 ,  // if set, is the return for if NAT will hairpin packets
             int port=0, // port to use for the test, 0 to choose random port
             StunAddress4* sAddr=0 // NIC to use 
   );

/// prints a StunAddress
std::ostream& 
operator<<( std::ostream& strm, const StunAddress4& addr);

std::ostream& 
operator<< ( std::ostream& strm, const UInt128& );

std::ostream& 
operator<< ( std::ostream& strm, const StunMsgHdr& );

bool
stunServerProcessMsg( char* buf,
                      unsigned int bufLen,
                      StunAddress4& from, 
                      StunAddress4& secondary,
                      StunAddress4& myAddr,
                      StunAddress4& altAddr, 
                      StunMessage* resp,
                      StunAddress4* destination,
                      StunAtrString* hmacPassword,
                      bool* changePort,
                      bool* changeIp,
                      bool verbose);

int
stunOpenSocket( StunAddress4& dest, 
                StunAddress4* mappedAddr, 
                int port=0, 
                StunAddress4* srcAddr=0, 
                bool verbose=false );

bool
stunOpenSocketPair( StunAddress4& dest, StunAddress4* mappedAddr, 
                    int* fd1, int* fd2, 
                    int srcPort=0,  StunAddress4* srcAddr=0,
                    bool verbose=false);

int
stunRandomPort();

void computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey);

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
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
 */

// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:

