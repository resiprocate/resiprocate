#include <string>
#include "rutil/TransportType.hxx"

namespace resip
{

static const Data transportNames[MAX_TRANSPORT] =
{
   Data("UNKNOWN_TRANSPORT"),
   Data("TLS"),
   Data("TCP"),
   Data("UDP"),
   Data("SCTP"),
   Data("DCCP"),
   Data("DTLS"),
   Data("WS"),
   Data("WSS")
};

static const Data transportNamesLower[MAX_TRANSPORT] =
{
   Data("UNKNOWN_TRANSPORT"),
   Data("tls"),
   Data("tcp"),
   Data("udp"),
   Data("sctp"),
   Data("dccp"),
   Data("dtls"),
   Data("ws"),
   Data("wss")
};

TransportType 
getTransportTypeFromName(const std::string& transportName)
{
    return toTransportType(transportName.c_str());
}

TransportType 
toTransportType(const resip::Data& transportName)
{
   for (TransportType i = UNKNOWN_TRANSPORT; i < MAX_TRANSPORT; 
        i = static_cast<TransportType>(i + 1))
    {
      if (isEqualNoCase(transportName, transportNames[i]))
    {
         return i;
    }
    }
    return UNKNOWN_TRANSPORT;
}

std::string 
getTransportNameFromType(const TransportType typeEnum)
{
   return toData(typeEnum).c_str();
}

std::string 
getTransportNameFromTypeLower(const TransportType typeEnum)
{
   return toDataLower(typeEnum).c_str();
}

const resip::Data& 
toData(const TransportType typeEnum)
{
   resip_assert(typeEnum >= UNKNOWN_TRANSPORT && typeEnum < MAX_TRANSPORT);
   return transportNames[typeEnum];
}

const resip::Data& 
toDataLower(const TransportType typeEnum)
{
   resip_assert(typeEnum >= UNKNOWN_TRANSPORT && typeEnum < MAX_TRANSPORT);
   return transportNamesLower[typeEnum];
}

bool
isReliable(TransportType type)
{
   switch(type)
   {
      case TLS:
      case TCP:
      case SCTP:
      case WS:
      case WSS:
         return true;
      case UDP:
      case DCCP:
      case DTLS:
      default:
         return false;
   }
}

bool
isSecure(TransportType type)
{
   switch(type)
   {
      case TLS:
      case DTLS:
      case WSS:
         return true;
      case UDP:
      case TCP:
      case DCCP:
      case SCTP:
      case WS:
      default:
         return false;
   }
}

bool
isWebSocket(TransportType type)
{
   switch(type)
   {
      case WS:
      case WSS:
         return true;
      default:
         return false;
   }
}

}

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
