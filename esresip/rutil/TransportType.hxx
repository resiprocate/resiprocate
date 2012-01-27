/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TRANSPORTTYPE_HXX)
#define RESIP_TRANSPORTTYPE_HXX

#include <ostream>
#include <string>
#include "rutil/Data.hxx"

namespace resip
{

/**
   @brief An enumeration of transport protocols.
   @ingroup network
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
   MAX_TRANSPORT
} TransportType;

/**
   @brief An enumeration of IP versions.
   @ingroup network
*/
typedef enum 
{
   V4,
   V6
} IpVersion;

/**
    @brief Function which translates a transport name to its corresponding 
      integer enum value.
    @param transportName the name of the transport e.g. "TCP", as a std::string
    @return the enum value for that transport
   @ingroup network
*/
TransportType getTransportTypeFromName(const std::string & transportName);

/**
    @brief Function which translates a transport enum value to its corresponding 
      name.
    @param transportNum the enum value of the transport
    @return the transport name, as a std::string
   @ingroup network
*/
std::string getTransportNameFromType(const TransportType typeEnum);
std::string getTransportNameFromTypeLower(const TransportType typeEnum);
const resip::Data& toDataLower(const TransportType typeEnum);

/**
    @brief Function which translates a transport enum value to its corresponding 
      name.
    @param transportNum the enum value of the transport
    @return the transport name, as a Data
   @ingroup network
*/
const resip::Data& toData(const TransportType typeEnum);

/**
    @brief Function which translates a transport name to its corresponding 
      integer enum value.
    @param transportName the name of the transport e.g. "TCP", as a Data
    @return the enum value for that transport
   @ingroup network
*/
TransportType toTransportType(const resip::Data & transportName);

bool isReliable(TransportType type);

bool isMultihoming(TransportType type);

bool isSupported(TransportType type, IpVersion version);

// Indicate whether or not to run a stun server on a Transport
typedef enum
{
   StunDisabled,
   StunEnabled
} StunSetting;
   
}

#endif

/* Copyright 2007 Estacado Systems */

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
