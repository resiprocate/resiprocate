#if !defined(RESIP_TRANSPORTTYPE_HXX)
#define RESIP_TRANSPORTTYPE_HXX

#include <ostream>
#include <string>
#include "rutil/Data.hxx"

namespace resip
{

/**
   @brief An enumeration of transport protocols.
*/
typedef enum 
{
   UNKNOWN_TRANSPORT = 0,
   TLS,
   TCP,
   UDP,
   SCTP,
   DCCP,
   DTLS,
   WS,
   WSS,
   MAX_TRANSPORT
} TransportType;

/**
   @brief An enumeration of IP versions.
*/
typedef enum 
{
   V4,
   V6
} IpVersion;

/**
    Function which translates a transport name to its corrisponding integer enum value.
    @param transportName the name of the transport e.g. "TCP"
    @return the enum value for that transport
**/
TransportType getTransportTypeFromName(const std::string & transportName);
TransportType toTransportType(const resip::Data & transportName);

/**
    Function which translates a transport enum value to its corrisponding name.
    @param transportNum the enum value of the transport
    @return the transport name

    @note Data version is more efficient and doesn't involve a copy
**/
std::string getTransportNameFromType(const TransportType typeEnum);
std::string getTransportNameFromTypeLower(const TransportType typeEnum);
const resip::Data& toData(const TransportType typeEnum);
const resip::Data& toDataLower(const TransportType typeEnum);

/// Returns true if passed in transport type is a reliable transport protocol
bool isReliable(TransportType type);

/// Returns true if passed in transport type is a secure transport protocol
bool isSecure(TransportType type);

/// Returns true if passed in transport type is a WebSocket transport protocol
bool isWebSocket(TransportType type);

// Indicate whether or not to run a stun server on a Transport
typedef enum
{
   StunDisabled,
   StunEnabled
} StunSetting;
   
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
