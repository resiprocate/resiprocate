#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <errno.h>

#include "resiprocate/os/Socket.hxx"

#include "resiprocate/Symbols.hxx"
#include "resiprocate/Uri.hxx"

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

#include "Resolver.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

using namespace resip;

Resolver::Resolver(const Uri& uri) : 
   mHost(uri.host()),
   mPort(uri.port() ? uri.port() : 5060)
{
   bool isNumeric = isIpAddress(mHost);
   if (!uri.exists(p_transport) )
   {
      if (isNumeric)
      {
         if (uri.scheme() == Symbols::Sip)
         {
            mTransport = UDP;
         }
         else if (uri.scheme() == Symbols::Sips)
         {
            mTransport = TCP;
         }
      }
      else // not numeric
      {
         if (1) // uri.portSpecified()) // !jf!
         {
            if (uri.scheme() == Symbols::Sip)
            {
               mTransport = UDP;
            }
            else if (uri.scheme() == Symbols::Sips)
            {
               mTransport = TCP;
            }
         }
         else // NAPTR query - yuck! 
         {
            // Rohan, step up to the plate buddy.
            mTransport = UDP; // !jf! not done yet
         }
      }
   }

   if (!isNumeric)
   {
      if (1) // !jf! uri.portSpecified())
      {
         lookupARecords();
         // do an A or AAAA DNS lookup
         
      }
      else
      {
         // do SRV lookup on result of NAPTR lookup
         
         // if no NAPTR lookup, do SRV lookup on _sips for sips and _sip for sip
         
         // if no result on SRV lookup, do an A or AAAA lookup
         
      }
   }
   else
   {
      Tuple tuple;
      if (inet_pton(AF_INET, mHost.c_str(), &tuple.ipv4.s_addr) <= 0)
      {
         DebugLog( << "inet_pton failed to parse address: " << mHost << " " << strerror(errno));
         assert(0);
      }
      tuple.port = mPort;
      tuple.transportType = mTransport;
      
      mNextHops.push_back(tuple);
   }
   
}


Resolver::Resolver(const Data& host, int port, TransportType transport) 
   :  mTransport(transport),
      mHost(host),
      mPort(port)
{
   bool isNumeric = isIpAddress(mHost);
   if (!isNumeric)
   {
      if (1) // !jf! uri.portSpecified())
      {
         lookupARecords();
         // do an A or AAAA DNS lookup
         
      }
      else
      {
         // do SRV lookup on result of NAPTR lookup
         
         // if no NAPTR lookup, do SRV lookup on _sips for sips and _sip for sip
         
         // if no result on SRV lookup, do an A or AAAA lookup
         
      }
   }
   else
   {
      Tuple tuple;
      if (inet_pton(AF_INET, mHost.c_str(), &tuple.ipv4.s_addr) <= 0)
      {
         DebugLog( << "inet_pton failed to parse address: " << mHost << " " << strerror(errno));
         assert(0);
      }
      tuple.port = mPort;
      tuple.transportType = mTransport;
      
      mNextHops.push_back(tuple);
   }
}


void
Resolver::lookupARecords()
{
   struct hostent hostbuf; 
   struct hostent* result=0;

   int herrno=0;
   char buffer[8192];
#ifdef __QNX__
   result = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno);
   if (result == 0)
   {
#else

#if defined( WIN32 ) || defined( __MACH__ ) || defined (__SUNPRO_CC) || defined(__FreeBSD__)
	assert(0); // !cj! 
	int ret = -1;
#else
        int ret = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
#endif
   assert (ret != ERANGE);

   if (ret != 0)
   {
#endif
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            InfoLog ( << "host not found: " << mHost);
            break;
         case NO_DATA:
            InfoLog ( << "no data found for: " << mHost);
            break;
         case NO_RECOVERY:
            InfoLog ( << "no recovery lookup up: " << mHost);
            break;
         case TRY_AGAIN:
            InfoLog ( << "try again: " << mHost);
            break;
      }
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
   
      DebugLog (<< "DNS lookup of " << mHost << ": canonical name: " << result->h_name);
      char str[256];
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         Tuple tuple;
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         tuple.port = mPort;
         tuple.transportType = mTransport;

         mNextHops.push_back(tuple);
#ifndef WIN32
         DebugLog (<< inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str)));
#endif

      }
   }
}

bool
Resolver::isIpAddress(const Data& data)
{
   // ok, this is fairly monstrous but it works. 
   unsigned int p1,p2,p3,p4;
   int count=0;
   int result = sscanf( data.c_str(), 
                        "%u.%u.%u.%u%n",
                        &p1, &p2, &p3, &p4, &count );

   if ( (result == 4) && (p1 <= 255) && (p2 <= 255) && (p3 <= 255) && (p4 <= 255) && (count == int(data.size())) )
   {
      return true;
   }
   else
   {
      return false;
   }
}
 

Data
Resolver::getHostName()
{
   char buffer[255];
   if (gethostname(buffer, sizeof(buffer)) < 0)
   {
      InfoLog (<< "Failed gethostname() " << strerror(errno));
      return "localhost";
   }
   else
   {
      return Data(buffer);
   }
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
