#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/ParserCategories.hxx"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#endif

// This is here so we can use the same macros to parse a dns result using the
// standard synchronous unix dns_query call
#if !defined(USE_ARES)
// borrowed from ares - will also work in win32

#define ARES_SUCCESS 0

#define DNS__16BIT(p)			(((p)[0] << 8) | (p)[1])
/* Macros for parsing a DNS header */
#define DNS_HEADER_QID(h)		DNS__16BIT(h)
#define DNS_HEADER_QR(h)		(((h)[2] >> 7) & 0x1)
#define DNS_HEADER_OPCODE(h)		(((h)[2] >> 3) & 0xf)
#define DNS_HEADER_AA(h)		(((h)[2] >> 2) & 0x1)
#define DNS_HEADER_TC(h)		(((h)[2] >> 1) & 0x1)
#define DNS_HEADER_RD(h)		((h)[2] & 0x1)
#define DNS_HEADER_RA(h)		(((h)[3] >> 7) & 0x1)
#define DNS_HEADER_Z(h)			(((h)[3] >> 4) & 0x7)
#define DNS_HEADER_RCODE(h)		((h)[3] & 0xf)
#define DNS_HEADER_QDCOUNT(h)		DNS__16BIT((h) + 4)
#define DNS_HEADER_ANCOUNT(h)		DNS__16BIT((h) + 6)
#define DNS_HEADER_NSCOUNT(h)		DNS__16BIT((h) + 8)
#define DNS_HEADER_ARCOUNT(h)		DNS__16BIT((h) + 10)

/* Macros for parsing the fixed part of a DNS resource record */
#define DNS_RR_TYPE(r)			DNS__16BIT(r)
#define DNS_RR_CLASS(r)			DNS__16BIT((r) + 2)
#define DNS_RR_TTL(r)			DNS__32BIT((r) + 4)
#define DNS_RR_LEN(r)			DNS__16BIT((r) + 8)

#endif


using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsResult::DnsResult(DnsInterface& interface) 
   : mInterface(interface),
     mSRVCount(0),
     mType(Pending)
{
}

bool
DnsResult::available() const
{
   return (mType == Available && !mResults.empty());
}

bool
DnsResult::finished()
{
   if (available())
   {
      return false;
   }
   else if (mType == Pending)
   {
      return false;
   }
   else if (!mSRVResults.empty())
   {
      mType = Pending;
      
      const SRV& srv = *mSRVResults.begin();
      mPort = srv.port;
      mTransport = srv.transport;
      lookupARecords(srv.target);
      mSRVResults.erase(mSRVResults.begin());
      return false;
   }
   else
   {
      return true;
   }
}

Transport::Tuple
DnsResult::next() 
{
   assert(available());
   Transport::Tuple next = mResults.front();
   mResults.pop_front();
   return next;
}

void
DnsResult::destroy()
{
   DebugLog (<< "DnsResult::destroy() " << this);
   
   if (mType == Pending)
   {
      mType = Destroyed;
   }
   else
   {
      delete this;
   }
}

void
DnsResult::lookup(const Uri& uri, const Data& tid)
{
   assert(uri.scheme() == Symbols::Sips || uri.scheme() == Symbols::Sip);  
   mTarget = uri.exists(p_maddr) ? uri.param(p_maddr) : uri.host();
   mSips = (uri.scheme() == Symbols::Sips);
   
   mTransactionId = tid;
   if (uri.exists(p_transport))
   {
      mTransport = Transport::toTransport(uri.param(p_transport));
   }
   else 
   {
      bool isNumeric = DnsUtil::isIpAddress(mTarget);
      if (isNumeric || uri.port() != 0)
      {
         if (mSips)
         {
            mTransport = Transport::TLS;
         }
         else 
         {
            mTransport = Transport::UDP;
         }

         if (isNumeric) // IP address specified
         {
            Transport::Tuple tuple;
            tuple.transportType = mTransport;
            tuple.transport = 0;
            inet_pton(AF_INET, mTarget.c_str(), &tuple.ipv4);
            tuple.port = uri.port();
            if (tuple.port == 0)
            {
               if (mSips) 
               {
                  mPort = 5061;
                  tuple.port = 5061;
               }
               else
               {
                  mPort = 5060;
                  tuple.port = 5060;
               }
            }
            mResults.push_back(tuple);
            mType = Available;
            mInterface.mHandler->handle(); // !jf! should I call this? 
         }
         else // port specified so we know the transport
         {
            mPort = uri.port();
            lookupARecords(mTarget); // for current target and port
         }
      }
      else // do NAPTR
      {
         lookupNAPTR(); // for current target
      }
   }
}

void
DnsResult::lookupARecords(const Data& target)
{
#if defined(USE_ARES)
   ares_gethostbyname(mInterface.mChannel, target.c_str(), AF_INET, DnsResult::aresHostCallback, this);
#else   
   struct hostent* result=0;
   int ret=0;
   int herrno=0;

#if defined(__linux__)
   struct hostent hostbuf; 
   char buffer[8192];
   ret = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
   assert (ret != ERANGE);
#elif defined(WIN32) 
   result = gethostbyname( host.c_str() );
   herrno = WSAGetLastError();
#elif defined( __MACH__ ) || defined (__FreeBSD__)
   result = gethostbyname( host.c_str() );
   herrno = h_errno;
#elif defined(__QNX__) || defined(__sun)
   struct hostent hostbuf; 
   char buffer[8192];
   result = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#else
#   error "need to define some version of gethostbyname for your arch"
#endif

   if ( (ret!=0) || (result==0) )
   {
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            InfoLog ( << "host not found: " << host);
            break;
         case NO_DATA:
            InfoLog ( << "no data found for: " << host);
            break;
         case NO_RECOVERY:
            InfoLog ( << "no recovery lookup up: " << host);
            break;
         case TRY_AGAIN:
            InfoLog ( << "try again: " << host);
            break;
		 default:
            ErrLog( << "DNS Resolver got error" << herrno << " looking up " << host );
            assert(0);
            break;
      }
   }
   else
   {
      processHost(ARES_SUCCESS, result);
   }
#endif
}

void
DnsResult::lookupNAPTR()
{
#if defined(USE_ARES)
   ares_query(mInterface.mChannel, mTarget.c_str(), C_IN, T_NAPTR, DnsResult::aresNAPTRCallback, this); 
#else
   unsigned char result[4096];
   int len = res_query(mTarget.c_str(), C_IN, T_NAPTR, result, sizeof(result));
   processNAPTR(ARES_SUCCESS, result, len);
#endif
}

void
DnsResult::lookupSRV(const Data& target)
{
#if defined(USE_ARES)
   ares_query(mInterface.mChannel, target.c_str(), C_IN, T_SRV, DnsResult::aresSRVCallback, this); 
#else
   unsigned char result[4096];
   int len = res_query(mTarget.c_str(), C_IN, T_SRV, result, sizeof(result));
   processNAPTR(ARES_SUCCESS, result, len);
#endif
}

#if defined (USE_ARES)
void
DnsResult::aresHostCallback(void *arg, int status, struct hostent* result)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received A result for: " << thisp->mTransactionId << " for " << thisp->mTarget);
   thisp->processHost(status, result);
}

void
DnsResult::aresNAPTRCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received NAPTR result for: " << thisp->mTransactionId << " for " << thisp->mTarget);
   thisp->processNAPTR(status, abuf, alen);
}


void
DnsResult::aresSRVCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received SRV result for: " << thisp->mTransactionId << " for " << thisp->mTarget);
   thisp->processSRV(status, abuf, alen);
}
#endif

void
DnsResult::processHost(int status, struct hostent* result)
{
   // There can only be one A/AAAA query outstanding at a time
   if (mType == Destroyed)
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      DebugLog (<< "DNS lookup canonical name: " << result->h_name);
      Transport::Tuple tuple;
      tuple.port = mPort;
      tuple.transportType = mTransport;
      tuple.transport = 0;
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         mResults.push_back(tuple);
      }
   }
   else
   {
      char* errmem=0;
      InfoLog (<< "Failed async dns query: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
   }

   mType = Available;
   mInterface.mHandler->handle();
}

void
DnsResult::processNAPTR(int status, unsigned char* abuf, int alen)
{
   // There can only be one NAPTR query outstanding at a time
   if (mType == Destroyed)
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      const unsigned char *aptr = abuf + HFIXEDSZ;
      int qdcount = DNS_HEADER_QDCOUNT(abuf);    /* question count */
      for (int i = 0; i < qdcount && aptr; i++)
      {
         aptr = skipDNSQuestion(aptr, abuf, alen);
      }
      
      int ancount = DNS_HEADER_ANCOUNT(abuf);    /* answer record count */
      for (int i = 0; i < ancount && aptr; i++)
      {
         NAPTR naptr;
         aptr = parseNAPTR(aptr, abuf, alen, naptr);

         if (aptr)
         {
            DebugLog (<< "Adding NAPTR record: " << naptr);
            if (mSips && naptr.service.find("SIPS"))
            {
               mNAPTRResults.insert(naptr);
            }
            else if (naptr.service == "SIPS+D2T" || 
                     naptr.service == "SIP+D2T" || 
                     naptr.service == "SIP+D2U")
            {
               mNAPTRResults.insert(naptr);
            }
         }
      }

      if (mNAPTRResults.empty())
      {
         InfoLog (<< "There are no NAPTR records so do an SRV lookup instead");
         goto NAPTRFail; // same as if no NAPTR records
      }

      mSRVCount++;
      const NAPTR& naptr = *mNAPTRResults.begin();
      lookupSRV(naptr.replacement);
      mNAPTRResults.erase(mNAPTRResults.begin());
   }
   else
   {
      {
         char* errmem=0;
         InfoLog (<< "NAPTR lookup failed: " << ares_strerror(status, &errmem));
         ares_free_errmem(errmem);
      }
      
      // This will result in no NAPTR results. In this case, send out SRV
      // queries for each supported protocol
     NAPTRFail:
      for (std::set<Transport::Type>::const_iterator i=mInterface.mSupportedTransports.begin(); 
           i != mInterface.mSupportedTransports.end(); i++)
      {
         Data target;
         DataStream strm(target);
         strm << "_sip.";
         switch(*i)
         {
            case Transport::UDP:
               strm << "_udp";
               break;
            case Transport::TCP:
               strm << "_tcp";
               break;
            case Transport::SCTP:
               strm << "_sctp";
               break;
            default:
               assert(0);
         }
         
         strm << ".";
         strm << mTarget;
         strm.flush();
         
         mSRVCount++;
         DebugLog (<< "Doing SRV query " << mSRVCount << " for " << target);
         lookupSRV(target);
      }
   }
}

void
DnsResult::processSRV(int status, unsigned char* abuf, int alen)
{
   mSRVCount--;

   // There could be multiple SRV queries outstanding
   if (mType == Destroyed && mSRVCount == 0)
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      const unsigned char *aptr = abuf + HFIXEDSZ;
      int qdcount = DNS_HEADER_QDCOUNT(abuf);    /* question count */
      for (int i = 0; i < qdcount && aptr; i++)
      {
         aptr = skipDNSQuestion(aptr, abuf, alen);
      }
      
      int ancount = DNS_HEADER_ANCOUNT(abuf);    /* answer record count */
      for (int i = 0; i < ancount && aptr; i++)
      {
         SRV srv;
         aptr = parseSRV(aptr, abuf, alen, srv);

         if (aptr)
         {
            DebugLog (<< "Adding SRV record: " << srv);
            mSRVResults.insert(srv);
         }
      }
   }
   else
   {
      char* errmem=0;
      InfoLog (<< "SRV lookup failed: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
   }

   if (mSRVCount == 0)
   {
      if (mSRVResults.empty())
      {
         if (mSips) 
         {
            mPort = 5061;
         }
         else
         {
            mPort = 5060;
         }
         mType = Pending;
         lookupARecords(mTarget); // for current target and port
      }
      else
      {
         const SRV& srv = *mSRVResults.begin();
         mPort = srv.port;
         mTransport = srv.transport;
         lookupARecords(srv.target);
         mSRVResults.erase(mSRVResults.begin());
      }
   }
}

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::skipDNSQuestion(const unsigned char *aptr,
                           const unsigned char *abuf,
                           int alen)
{
   char *name=0;
   int status=0;
   int len=0;
   
   // Parse the question name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      return NULL;
   }
   aptr += len;

   // Make sure there's enough data after the name for the fixed part
   // of the question.
   if (aptr + QFIXEDSZ > abuf + alen)
   {
      free(name);
      return NULL;
   }

   // Parse the question type and class. 
   //int type = DNS_QUESTION_TYPE(aptr);
   //int dnsclass = DNS_QUESTION_CLASS(aptr);
   aptr += QFIXEDSZ;
   
   free(name);
   return aptr;
}

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseSRV(const unsigned char *aptr,
                    const unsigned char *abuf, 
                    int alen,
                    SRV& srv)
{
   char *name;
   int len=0;
   int status=0;

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      return NULL;
   }
   aptr += len;

   /* Make sure there is enough data after the RR name for the fixed
    * part of the RR.
    */
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      free(name);
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      free(name);
      return NULL;
   }
   free(name);

   // Display the RR data.  Don't touch aptr. 
   if (type == T_SRV)
   {
      // The RR data is three two-byte numbers representing the
      // priority, weight, and port, followed by a domain name.
      srv.priority = DNS__16BIT(aptr);
      srv.weight = DNS__16BIT(aptr + 2);
      srv.port = DNS__16BIT(aptr + 4);
      status = ares_expand_name(aptr + 6, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         return NULL;
      }
      srv.target = name;
      free(name);

      return aptr + dlen;

   }
   else
   {
      return NULL;
   }
}
      
// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseNAPTR(const unsigned char *aptr,
                      const unsigned char *abuf, 
                      int alen,
                      NAPTR& naptr)
{
   const unsigned char* p=0;
   char* name=0;
   int len=0;
   int status=0;

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      return NULL;
   }
   aptr += len;

   // Make sure there is enough data after the RR name for the fixed
   // part of the RR.
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      free(name);
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      free(name);
      return NULL;
   }
   free(name);

   // Look at the RR data.  Don't touch aptr. 
   if (type == T_NAPTR)
   {
      // The RR data is two two-byte numbers representing the
      // order and preference, followed by three character strings
      // representing flags, services, a regex, and a domain name.

      naptr.order = DNS__16BIT(aptr);
      naptr.pref = DNS__16BIT(aptr + 2);
      p = aptr + 4;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         return NULL;
      }
      naptr.flags = Data(p+1, len);
      
      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         return NULL;
      }
      naptr.service = Data(p+1, len);

      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         return NULL;
      }
      naptr.regex = Data(p+1, len);

      p += len + 1;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         return NULL;
      }
      naptr.replacement = Data(p+1, len);
      
      free(name);
      return aptr + dlen;
   }
   else
   {
      return NULL;
   }
}

/* 
   Copyright (c) 2003, Jason Fischl
 
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
    
   * Neither the name of any of the copyright holders nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.
    
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
*/
