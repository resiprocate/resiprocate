#ifndef WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <WS2TCPIP.H>
#endif

#include "resip/stack/SipStack.hxx"
#include "rutil/Logger.hxx"
#include "tfm/Resolver.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

using namespace resip;


//.dcm.  I think this is where we can modify the resolution logic to be more
//robust and default to the particular testproxy/fake srv.
Resolver::Resolver(const Uri& uri) : 
   mTransport(UNKNOWN_TRANSPORT),
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
   else
   {
      mTransport = Tuple::toTransport(uri.param(p_transport));
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
      Tuple tuple(mHost, mPort, mTransport);
      mNextHops.push_back(tuple);
   }
}


Resolver::Resolver(const resip::Data& host, int port, TransportType transport) 
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
      Tuple tuple(mHost, mPort, mTransport);
      mNextHops.push_back(tuple);
   }
}


void
Resolver::lookupARecords()
{
   struct hostent hostbuf; 
   struct hostent* result;

   int herrno=0;
   char buffer[8192];
#ifdef __QNX__
   result = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno);
   if (result == 0)
   {
#else

#ifdef WIN32
	result = gethostbyname(mHost.c_str());
    int ret = (result==0);
#elif defined(__NetBSD__)
    //!dcm! -- not threadsafe
    result = gethostbyname(mHost.c_str());    
    int ret = errno;    
#elif defined(__APPLE__)
    // gethostbyname in os/x is thread-safe...
    // http://developer.apple.com/technotes/tn2002/pdf/tn2053.pdf
    result = gethostbyname(mHost.c_str());
    int ret = (result==0);
#else
   int ret = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
#endif
   assert (ret != ERANGE);
   if (ret != 0)
   {
#endif

#ifdef WIN32
      InfoLog(<< "gethostbyname error: " << WSAGetLastError());
#else
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
#endif
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
   
      DebugLog (<< "DNS lookup of " << mHost << ": canonical name: " << result->h_name);
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         in_addr addr;
         addr.s_addr = *((u_int32_t*)(*pptr)); 
         Tuple tuple(addr, mPort, mTransport);
         mNextHops.push_back(tuple);
      }
   }
}

bool
Resolver::isIpAddress(const resip::Data& data)
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
 
resip::Data
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
      return resip::Data(buffer);
   }
}
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
