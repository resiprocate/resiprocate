/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

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

/** 
    @file 
    @brief STUN and TURN implementation.
*/

/** @name Compatibility-related typedefs */
//@{ 
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

//@}

/// Portable unsigned 128 bit integer
typedef struct { unsigned char octet[16]; }  UInt128;

/// @name Portable unsigned 128 bit integer operators
//@{ 

#ifdef __cplusplus
bool operator<(const UInt128&, const UInt128&);
bool operator==(const UInt128&, const UInt128&);
#endif
//@}


/// define a structure to hold a stun address 
//@{ 
const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;
//@}

/// STUN CHANGE-REQUEST flags
//@{ 
const UInt32 ChangeIpFlag   = 0x04;
const UInt32 ChangePortFlag = 0x02;
//@}

/** @name STUN Attributes */
//@{ 
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
//@}

// !jf! TURN specific message attributes - from turn-08
/** @name TURN Attributes (midcom-turn-08) */
//@{ 
const UInt16 TurnLifetime     = 0x000d;
const UInt16 TurnAlternateServer = 0x000e;
const UInt16 TurnMagicCookie = 0x000f;
const UInt16 TurnBandwidth = 0x0010;
const UInt16 TurnDestinationAddress = 0x0011;
const UInt16 TurnRemoteAddress = 0x0012;
const UInt16 TurnData = 0x0013;
const UInt16 TurnNonce = 0x0014;
const UInt16 TurnRealm = 0x0015;
//@}


/// @name STUN message types 
//@{ 
const UInt16 BindRequestMsg               = 0x0001;
const UInt16 BindResponseMsg              = 0x0101;
const UInt16 BindErrorResponseMsg         = 0x0111;
const UInt16 SharedSecretRequestMsg       = 0x0002;
const UInt16 SharedSecretResponseMsg      = 0x0102;
const UInt16 SharedSecretErrorResponseMsg = 0x0112;
//@}


/** @name TURN message types (midcom-turn-08) */
//@{ 
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
//@}

/**
   The header at the beginning of every STUN message has the
   type of the message (e.g. Binding Request), the length of the
   message in octets, and a 128 transaction identifier.
*/
typedef struct 
{
      UInt16 msgType;
      UInt16 msgLength;
      UInt128 id;
} StunMsgHdr;

#ifdef __cplusplus
/// Compares the transaction IDs of the STUN headers.
bool operator<(const StunMsgHdr&, const StunMsgHdr&);
#endif

/**
   The header of each attribute in a STUN message conveys the type
   (ID) of the attribute, and it's length.
*/
typedef struct
{
      UInt16 type;
      UInt16 length;
} StunAtrHdr;

/**
   The IPv4 32-bit address and 16-bit port number.
*/
typedef struct
{
      UInt16 port;
      UInt32 addr;
} StunAddress4;

/**
   Generically represents any of the *ADDRESS attributes that could be
   present in a STUN message.
*/
typedef struct
{
      UInt8 pad;
      UInt8 family;
      StunAddress4 ipv4;
} StunAtrAddress4;

/**
   The CHANGE-REQUEST attribute.
*/
typedef struct
{
      UInt32 value;
} StunAtrChangeRequest;

/**
   The ERROR-CODE attribute.
*/
typedef struct
{
      UInt16 pad; // all 0
      UInt8 errorClass;
      UInt8 number;
      char reason[STUN_MAX_STRING];
      UInt16 sizeReason;
} StunAtrError;

/**
   The UNKNOWN-ATTRIBUTES attribute.
*/
typedef struct
{
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      UInt16 numAttributes;
} StunAtrUnknown;

/**
   Generically represents any string-ish, variable-length attribute.
*/
typedef struct
{
      char value[STUN_MAX_STRING];      
      UInt16 sizeValue;
} StunAtrString;

/**
   The MESSAGE-INTEGRITY attribute.
*/
typedef struct
{
      char hash[20];
} StunAtrIntegrity;


enum StunHmacStatus
{
   HmacUnkown=0,
   HmacOK,
   HmacBadUserName,
   HmacUnkownUserName,
   HmacFailed
};

/**
   A single STUN message has a mandatory header.  It usually contains
   additional attributes which may mandatory or optional for a given
   message type.
*/
typedef struct
{
      /// The header that is included in all STUN messages.
      StunMsgHdr msgHdr;
	
      bool hasMappedAddress;
      /// MAPPED-ADDRESS attribute.
      StunAtrAddress4  mappedAddress;
	
      bool hasResponseAddress;
      /// RESPONSE-ADDRESS attribute.
      StunAtrAddress4  responseAddress;
	
      bool hasChangeRequest;
      /// CHANGE-REQUEST attribute.
      StunAtrChangeRequest changeRequest;
	
      bool hasSourceAddress;
      /// SOURCE-ADDRESS attribute.
      StunAtrAddress4 sourceAddress;
	
      bool hasChangedAddress;
      /// CHANGED-ADDRESS attribute.
      StunAtrAddress4 changedAddress;
	
      bool hasUsername;
      /// USERNAME attribute.
      StunAtrString username;
	
      bool hasPassword;
      /// PASSWORD attribute.
      StunAtrString password;
	
      bool hasMessageIntegrity;
      /// MESSAGE-INTEGRITY attribute.
      StunAtrIntegrity messageIntegrity;
	
      bool hasErrorCode;
      /// ERROR-CODE attribute.
      StunAtrError errorCode;
	
      bool hasUnknownAttributes;
      /// UNKNOWN-ATTRIBUTES attribute.
      StunAtrUnknown unknownAttributes;
	
      bool hasReflectedFrom;
      /// REFLECTED-FROM attribute.
      StunAtrAddress4 reflectedFrom;

      bool hasXorMappedAddress;
      /// XOR-MAPPED-ADDRESS attribute.
      StunAtrAddress4  xorMappedAddress;
	
      bool xorOnly;

      bool hasServerName;
      /// SERVER-NAME attribute.
      StunAtrString serverName;
      
      bool hasSecondaryAddress;
      /// SECONDARY-ADDRESS attribute.
      StunAtrAddress4 secondaryAddress;

      bool hasTurnLifetime;
      /// LIFETIME attribute. TURN only.
      UInt32 turnLifetime;
      
      bool hasTurnAlternateServer;
      ///  ALTERNATE-SERVER attribute. TURN only.
      StunAtrAddress4 turnAlternateServer;
      
      bool hasTurnMagicCookie;
      ///  MAGIC-COOKIE attribute. TURN only.
      UInt32 turnMagicCookie;
      
      bool hasTurnBandwidth;
      ///  BANDWIDTH attribute. TURN only.
      UInt32 turnBandwidth;
      
      bool hasTurnDestinationAddress;
      ///  DESTINATION-ADDRESS attribute. TURN only.
      StunAtrAddress4 turnDestinationAddress;
      
      bool hasTurnRemoteAddress;
      ///  REMOTE-ADDRESS attribute. TURN only.
      StunAtrAddress4 turnRemoteAddress;
      
      bool hasTurnData;
      /// Carries the contents of the DATA attribute of a TURN
      /// message, if it was included.
      resip::Data* turnData;
      
      //bool hasTurnNonce;
      // turnNonce;
      
      //bool hasTurnRealm;
      // turnRealm;
} StunMessage; 


/// @enum NatType
/// NAT types according to RFC 4787.
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

/**
   Stores information about a relay that has been set up in a TURN
   server.
*/
typedef struct 
{
      int relayPort;       ///< media relay port
      int fd;              ///< media relay file descriptor
      StunAddress4 destination; ///< NAT IP:port
      time_t expireTime;      ///< if no activity after time, close the socket 
} StunMediaRelay;


/**
   Stores information about the STUN/TURN server.  
*/
typedef struct
{
      StunAddress4 myAddr;
      StunAddress4 altAddr;
      resip::Socket myFd;
      resip::Socket altPortFd;
      resip::Socket altIpFd;
      resip::Socket altIpPortFd;
      /// true if media relaying is to be done
      bool relay; 
      
      /// The relays that have been set up on this TURN server.
      StunMediaRelay relays[MAX_MEDIA_RELAYS];
} StunServerInfo;


/**
   @brief Parses a buffer into a StunMessage.
   @param buf The character buffer holding the encoded STUN message.
   @param bufLen The length of buf in octets.
   @param message OUTPUT parameter holding the decoded StunMessage.
   @param verbose If TRUE, dumps extra logging.
   @retval False if the message failed in parsing.
*/
bool
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage& message, 
                  bool verbose );


/**
   @brief Build a simple BindingRequest.
   
   A StunMessage is passed in.  The function transforms the message
   into a valid BindingRequest. A CHANGE-REQUEST attribute will be
   added to the message.

   @param msg Pointer to the message which will be transformed.
   
   @param username If username size > 0, it will be used as the STUN
   message's USERNAME attribute.

   @param changePort Sets the "change port" flag of the CHANGE-REQUEST
   attribute.

   @param changeIp Sets the "change IP" flag of the CHANGE-REQUEST
   attribute.

   @param id If non-zero, this is used as the least significant octet
   of the id in the StunMsgHdr.
*/
void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString& username,
                    bool changePort, bool changeIp, unsigned int id=0 );


/**
   @brief Encode a StunMessage to a buffer.

   @param message The STUN message to encode.

   @param buf The buffer to which the message will be encoded.

   @param bufLen  The size of the buffer.
*/
unsigned int
stunEncodeMessage( const StunMessage& message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString& password,
                   bool verbose);

/**
   @brief Creates a new user name based on IP address, system time,
   and a random number.

   The concatenation of a IP Address, the random number and system
   time are then hashed.  The HMAC-SHA1 hash of this value is
   represented in it's 40-octet form (the ASCII representation of the
   hex result).

   @param addr The IP address to use in the initial concatenation.
   
   @param username The output parameter which will hold the computed
   username
*/
void
stunCreateUserName(const StunAddress4& addr, StunAtrString* username);

/**
   @brief Create a new username and password.

   Calls stunCreateUserName() and stunCreatePassword() to create both
   a username and a password.

   @param dest The IP address that will be passed to stunCreateUserName().

   @param username Output parameter holding the newly created username.

   @param password Output parameter holding the newly created password.
   
*/
void 
stunGetUserNameAndPassword(  const StunAddress4& dest, 
                             StunAtrString* username,
                             StunAtrString* password);

/**
   @brief Creates a new password for a given username.

   An HMAC-SHA1 hash is computed over the username.  The result is the
   new password.

   @param username The username used to make the new password.

   @param password The output parameter which will hold the newly
   computed password.
*/
void
stunCreatePassword(const StunAtrString& username, StunAtrString* password);

/**
   @brief Generates a 32 bit random number.
*/
int 
stunRand();

/**
   @brief Gets system time in seconds.

   @retval The number of seconds, figured as 3600*hours + 60 minutes +
   seconds.
*/
UInt64
stunGetSystemTimeSecs();

/**
   Parse the IP address of a the specified stun server.

   @param serverName IP address and port of the form "A.B.C.D:Port"
   
   @param stunServerAddr The output parameter which will hold the
   parsed address.
   
   @retval false if fails parse.
*/
bool  
stunParseServerName( char* serverName, StunAddress4& stunServerAddr);

/**
   Parse the string representation of a host name into a struct
   StunAddress4.

   @param peerName IP address and port of the form "A.B.C.D:Port"

   @param ip Output parameter holding the parsed IP address.

   @param portVal Output parameter holding the parsed port number.

   @param defaultPort If not port is specified in peerName, this port
   will be used for portVal.
   
   @retval false if fails parse.
*/
bool 
stunParseHostName( char* peerName,
                   UInt32& ip,
                   UInt16& portVal,
                   UInt16 defaultPort );

/**
   @deprecated unused
*/
bool
stunInitServer(StunServerInfo& info, 
               const StunAddress4& myAddr, 
               const StunAddress4& altAddr,
               int startMediaPort,
               bool verbose);



/**
   @deprecated unused

   Used only by stunInitServer(), which is also deprecated.
*/
void
stunStopServer(StunServerInfo& info);

/**
   Makes a read fd_set from the the server info, polls [calls select()] the active
   sockets, and then reads and processes any incoming messages.

   @param info The sockets which will be polled for activity.

   @retval Return true if all is OK 
*/
bool
stunServerProcess(StunServerInfo& info, bool verbose);

/**
   @brief Discover the IP interfaces configured in the host operating
   system.

   @note Not supported for Windows or SPARC and will always return 0.

   @param addresses An output parameter treated as an array of 32-bit
   ints.  These ints are actually cast from sin_addr.s_addr (struct
   sockaddr_in).

   @param maxSize The maximum number of results which will be retrieved.

   @returns Number of addresses found.
*/
int 
stunFindLocalInterfaces(UInt32* addresses, int maxSize );

/**
   @deprecated Unused.  
*/
bool 
stunTest( StunAddress4& dest, int testNum, bool verbose, StunAddress4* srcAddr=0, unsigned long timeoutMs=5000 );

/**
   @deprecated This implements the RFC 3489 NAT discovery algorithm,
   which is deprecated by RFC 3489bis (draft).

   @param dest The address of the STUN server.

   @param verbose True if verbose logging is desired.

   @param preservePort The output parameter which indicates whether
   the NAT which is between this host and the STUN server preserves
   port numbers.

   @param hairpin The output parameter which indicates whether the NAT
   which is between this host and the STUN server supports
   hairpinning.

   @param sAddr The desired NIC to send from.

*/
NatType
stunNatType( StunAddress4& dest, bool verbose, 
             bool* preservePort=0, // if set, is return for if NAT preservers ports or not
             bool* hairpin=0 ,  // if set, is the return for if NAT will hairpin packets
             int port=0, // port to use for the test, 0 to choose random port
             StunAddress4* sAddr=0 // NIC to use 
   );

/// Prints a StunAddress.
std::ostream& 
operator<<( std::ostream& strm, const StunAddress4& addr);

/// Prints an unsigned 128 bit integer.
std::ostream& 
operator<< ( std::ostream& strm, const UInt128& );


/// Prints a StunMsgHdr.
std::ostream& 
operator<< ( std::ostream& strm, const StunMsgHdr& );

/**
   @brief Server processing for STUN messages.

   This implements STUN server processing for SharedSecretRequest and
   BindingRequest messages.

   The message is parsed and handled. A response is formulated, and is
   returned in output parameter, but not sent. The parsed request is
   not returned to the caller.

   @param buf The buffer containing the unparsed STUN message.

   @param bufLen The number of bytes in the buffer.

   @param from The address the message was received from.

   @param secondary This isn't currently used.

   @param myAddr The address this request was received on.

   @param altAddr If the request is a BindingRequest with a
   CHANGE-REQUEST attribute, this contains the IP or port the address
   will be changed TO.

   @param resp The output parameter that holds the response to the
   incoming message.

   @param destination The output parameter that contains the
   destination the resp message should be sent to.

   @param hmacPassword The output parameter that holds the result of
   stunCreatePassword().  If the request uses MESSAGE-INTEGRITY, the
   response usually must have one also. 

   @param changePort The output parameter which indicates if the
   "change port" flag was included in a CHANGE-REQUEST attribute.

   @param changeIp The output parameter which indicates if the "change
   IP" flag was included in a CHANGE-REQUEST attribute.

   @param verbose If true, emit verbose logging.
   
*/
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

/**
   @brief Open a socket and discover the address mapped by the NAT.

   This opens a socket, creates and sends a BindingRequest, waits for
   the result.  The mapped address is returned to the caller.

   @param dest The destination of the STUN server.
   
   @param mappedAddr The output parameter which holds the result of
   MAPPED-ADDRESS or XOR-MAPPED-ADDRESS sent in the BindingResponse
   from the server.
   
   @param port The port on which to bind the socket.  Use 0 to choose
   a port randomly.

   @param srcAddr The address to which to bind the socket.  Use 0 to
   choose INADDR_ANY.

   @param verbose If true, emit verbose logging.

   @returns The Socket fd is returned, cast as an int.
   @retval -1 indicates failure.

*/
int
stunOpenSocket( StunAddress4& dest, 
                StunAddress4* mappedAddr, 
                int port=0, 
                StunAddress4* srcAddr=0, 
                bool verbose=false );


/**
   @brief Open a pair of sockets and discover their mapped addresses. 

   This attempts to find an address and port pair which have
   consecutive port numbers and whose mapped addresses have port
   numbers which also are consecutive.

   Three sockets will be attempted: at port, port+1 and port+2.

   @param dest The destination of the STUN server.
   
   @param mappedAddr The output parameter which holds the mapped
   address of the socket with the even port.

   @param fd1 The output parameter holding the first socket of the pair.

   @param fd2 The output parameter holding the second socket of the pair.
   
   @param srcPort The port on which to bind the sockets.  Use 0 to choose
   a port randomly. Port numbers port, port+1, and port+2 will be
   tried.

   @param srcAddr The address to which to bind the socket.  Use 0 to
   choose INADDR_ANY.

   @param verbose If true, emit verbose logging.

   @retval False indicates failure.  Any relevant sockets are
   automatically closed in this case.
*/
bool
stunOpenSocketPair( StunAddress4& dest, StunAddress4* mappedAddr, 
                    int* fd1, int* fd2, 
                    int srcPort=0,  StunAddress4* srcAddr=0,
                    bool verbose=false);


/**
   @brief Choose a random port number.

   @returns A random port number between 16384 and 32767 inclusive.
*/
int
stunRandomPort();

/**
   @deprecated Not implemented.

   Computes a SHA-1 HMAC over the input buffer.

   @param hmac The output parameter which holds the result.  Note that
   SHA-1 is 160 bits, so this buffer needs to be 20 bytes long.

   @param input The input buffer.

   @param lenght The length of the input.

   @param key The HMAC key.

   @param sizeKey The size of the key in bytes.

*/
void computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey);

#endif


/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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

