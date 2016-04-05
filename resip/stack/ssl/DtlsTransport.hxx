
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#if !defined(RESIP_DTLSTRANSPORT_HXX)
#define RESIP_DTLSTRANSPORT_HXX

#ifdef USE_DTLS

#ifndef RESIP_UDPTRANSPORT_HXX
#include "resip/stack/UdpTransport.hxx"
#endif

#ifndef RESIP_TIMERQUEUE_HXX
#include "resip/stack/TimerQueue.hxx"
#endif

#ifndef RESIP_SENDDATA_HXX
#include "resip/stack/SendData.hxx"
#endif

#ifndef RESIP_HEAPINSTANCECOUNTER_HXX
#include "rutil/HeapInstanceCounter.hxx"
#endif

#ifndef RESIP_HASHMAP_HXX
#include "rutil/HashMap.hxx"
#endif

#include <map>
#include <openssl/ssl.h>

#include "resip/stack/Compression.hxx"

namespace resip
{

class Security;
class TransactionMessage;
class UdpTransport;
class DtlsMessage;

class DtlsTransport : public UdpTransport
{
#if  defined(__INTEL_COMPILER ) || (defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1310) && (_MSC_VER < 1900))  // !slg! not sure if this works on __INTEL_COMPILER 
   struct sockaddr_in_hash_compare
   {
      enum { bucket_size = 4, min_buckets = 8 };

      size_t operator()(const struct sockaddr_in& sock) const 
      { 
          return sock.sin_addr.s_addr; 
      }
      bool operator()(const struct sockaddr_in& s1, 
                      const struct sockaddr_in& s2) const
      {
         if ( (s1.sin_addr.s_addr < s2.sin_addr.s_addr) ||
              ( (s1.sin_addr.s_addr == s2.sin_addr.s_addr ) &&
                ( s1.sin_port < s2.sin_port) ) )
         {
             return 1;
         }
         else
         {
            return 0;
         }
      }
   };
#elif defined(HASH_MAP_NAMESPACE)
   struct addr_hash
   {
      size_t operator()( const struct sockaddr_in sock ) const
      {
         return sock.sin_addr.s_addr ;
      }            
   };
     
   struct addr_cmp
   { 
      bool operator()(const struct sockaddr_in& s1, 
                      const struct sockaddr_in& s2) const
      {
         if ( ( s1.sin_addr.s_addr == s2.sin_addr.s_addr ) &&
              ( s1.sin_port == s2.sin_port) )
         {
            return 1;
         }
         else
         {
            return 0;
         }
      }
   };
#else
   struct addr_less
   { 
      bool operator()(const struct sockaddr_in& s1, 
                      const struct sockaddr_in& s2) const
      {
         if ( (s1.sin_addr.s_addr < s2.sin_addr.s_addr) ||
              ( (s1.sin_addr.s_addr == s2.sin_addr.s_addr ) &&
                ( s1.sin_port < s2.sin_port) ) )
         {
             return 1;
         }
         else
         {
            return 0;
         }
      }
   };
#endif

   public:
      RESIP_HeapCount(DtlsTransport);
      // Specify which udp port to use for send and receive
      // interface can be an ip address or dns name. If it is an ip address,
      // only bind to that interface.
      DtlsTransport(Fifo<TransactionMessage>& fifo,
                    int portNum,
                    IpVersion version,
                    const Data& interfaceObj,
                    Security& security,
                    const Data& sipDomain,
                    AfterSocketCreationFuncPtr socketFunc = 0,
                    Compression &compression = Compression::Disabled,
                    const Data& certificateFilename = "", 
                    const Data& privateKeyFilename = "",
                    const Data& privateKeyPassPhrase = "");
      virtual  ~DtlsTransport();

      void process(FdSet& fdset);
      bool isReliable() const { return false; }
      bool isDatagram() const { return true; }
      virtual void buildFdSet( FdSet& fdset);

      static const unsigned long DtlsReceiveTimeout = 250000 ;

   private:

#if  defined(__INTEL_COMPILER ) || (defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1310) && (_MSC_VER < 1900))
      typedef HashMap<struct sockaddr_in, 
                      SSL*, 
                      DtlsTransport::sockaddr_in_hash_compare> DtlsConnectionMap;
#elif defined(HASH_MAP_NAMESPACE)
      typedef HashMap<struct sockaddr_in, 
                      SSL*, 
                      DtlsTransport::addr_hash, 
                      DtlsTransport::addr_cmp> DtlsConnectionMap ;
#else
      typedef std::map<struct sockaddr_in, 
                      SSL*, 
                      DtlsTransport::addr_less> DtlsConnectionMap ;
#endif

      SSL_CTX             *mClientCtx ;
      SSL_CTX             *mServerCtx ;
      MsgHeaderScanner    mMsgHeaderScanner;
      static const int    MaxBufferSize;
      Fifo<DtlsMessage>   mHandshakePending ;
      DtlsTimerQueue      mTimer ;
      Security*           mSecurity ;
      DtlsConnectionMap   mDtlsConnections ;  /* IP addr/port -> transport */
      unsigned char       mDummyBuf[ 4 ] ;
      BIO*                mDummyBio ;
      const Data          mDomain;

      SendData            *mSendData ;/* Data that was unqueued from mTxFifo, 
                                       * but unable to send because a handshake
                                       * was in progress 
                                       */
      
      void _read( FdSet& fdset ) ;
      void _write( FdSet& fdset ) ;
      void _doHandshake() ;
      void _cleanupConnectionState( SSL *ssl, struct sockaddr_in peer ) ;

      void _mapDebug( const char *where, const char *action, SSL *ssl ) ;
      void _printSock( const struct sockaddr_in *sock ) ;
};

}

#endif /* USE_DTLS */

#endif /* ! RESIP_DTLSTRANSPORT_HXX */

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
