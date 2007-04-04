#include "rutil/TransportType.hxx"
#include <string>

namespace resip
{
TransportType getTransportTypeFromName(const std::string & transportName)
{
    if( transportName == "UDP" )
    {
        return UDP;
    }
    if( transportName == "TCP" )
    {
        return TCP;
    }
    if( transportName == "TLS" )
    {
        return TLS;
    }
    if( transportName == "SCTP" )
    {
        return SCTP;
    }
    if( transportName == "DCCP" )
    {
        return DCCP;
    }
    if( transportName == "DTLS" )
    {
        return DTLS;
    }
    return UNKNOWN_TRANSPORT;
}

std::string getTransportNameFromType(const TransportType typeEnum)
{
    switch(typeEnum)
    {
        case UDP:  return "UDP";
        case TCP:  return "TCP";
        case TLS:  return "TLS";
        case SCTP: return "SCTP";
        case DCCP: return "DCCP";
        case DTLS: return "DTLS";
        default: return "UNKNOWN";
    }
    return "UNKNOWN_TRANSPORT";
}

resip::Data toData(const TransportType typeEnum)
{
   switch(typeEnum)
   {
      case UNKNOWN_TRANSPORT:
         return resip::Data("UNKNOWN_TRANSPORT");
      case UDP:
         return resip::Data("UDP");
      case TCP:
         return resip::Data("TCP");
      case TLS:
         return resip::Data("TLS");
      case SCTP:
         return resip::Data("SCTP");
      case DCCP:
         return resip::Data("DCCP");
      case DTLS:
         return resip::Data("DTLS");
      default:
         return resip::Data("UNKNOWN");
   }
}

TransportType toTransportType(const resip::Data & transportName)
{
   if(transportName=="UDP")
   {
      return resip::UDP;
   }
   else if(transportName=="TCP")
   {
      return resip::TCP;
   }
   else if(transportName=="TLS")
   {
      return resip::TLS;
   }
   else if(transportName=="DTLS")
   {
      return resip::DTLS;
   }
   else if(transportName=="SCTP")
   {
      return resip::SCTP;
   }
   else if(transportName=="DCCP")
   {
      return resip::DCCP;
   }
   else
   {
      return resip::UNKNOWN_TRANSPORT;      
   }
   

}

bool
isReliable(TransportType type)
{
   switch(type)
   {
      case TLS:
      case TCP:
      case SCTP:
         return true;
      case UDP:
      case DCCP:
      case DTLS:
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
