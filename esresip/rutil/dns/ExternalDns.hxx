/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_EXTERNAL_DNS_HXX)
#define RESIP_EXTERNAL_DNS_HXX

#include <vector>

#include "rutil/AsyncID.hxx"
#include "rutil/GenericIPAddress.hxx"

struct hostent;

namespace resip
{
class ExternalDnsHandler;
class ExternalDnsRawResult;
class ExternalDnsHostResult;

//used by the asynchronous executive

/**
   @brief Base class for DNS backend.

   Application writers can write their own subclass of this, but chances are 
   good that AresDns will work fine in most environments.

   @ingroup resip_dns
*/
class ExternalDns
{
   public:
      enum Features
      {
         None = 0,
         TryServersOfNextNetworkUponRcode3 = 1 << 0   // 'No such name'
      };

      //returns Success, BuildMismatch, otherwise ExternalDns specific 
      //error message can be pulled from errorMessage
      enum InitResult 
      {
         Success = 0,
         BuildMismatch = 4777
      };      
      
      /**
         @brief Initializes the DNS backend.

         Implementation is expected to do the following:
            - Configure the backend to search the additional nameservers in
               additionalNameservers.
            - Configure the backend to use the timeout provided in dnsTimeout, 
               if non-zero, and use a default value if zero.
            - Configure the backend to try each lookup the number of times 
               specified by dnsTries.
            - Invoke the AfterSocketCreationFuncPtr once the DNS backend has 
               aquired a socket.

         @param additionalNameservers The additional nameservers the backend is 
            expected to search for each query.
         @param func A function-pointer to invoke when the backend has acquired 
            a socket.
         @param dnsTimeout Timeout for queries, in seconds.
         @param dnsTries Number of times queries should be retried if they fail.
         @return 0 for success, non-zero for errors (see errorMessage())
      */
      virtual int init(const std::vector<GenericIPAddress>& additionalNameservers,
                       AfterSocketCreationFuncPtr func,
                       int dnsTimeout = 0,
                       int dnsTries = 0,
                       unsigned int features = 0) = 0; // bit mask of Features

      /**
         @brief For use in select timeout.
      */
      //For use in select timeout
      virtual unsigned int getTimeTillNextProcessMS() = 0;

      //returns 'true' only is there are changes in the DNS server list
      virtual bool checkDnsChange() = 0;

      //this is scary on windows; the standard way to get a bigger fd_set is to
      //redefine FD_SETSIZE befor each inclusion of winsock2.h, so make sure
      //external libraries have been properly configured      
      /**
         @brief Make the backend place the file-descriptors it is waiting for
            in read and write.
         
            After this function returns, the user will do a select on the 
            fd_sets that were passed, and call process() when the select() 
            returns.
         
         @param read The backend will add any file descriptors that it is 
            waiting to read from into this fd_set.
         @param write The backend will add any file descriptors it is waiting to 
            write to into this fd_set.
         @param size The number of file descriptors added will be set here.
      */
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size) = 0;

      /**
         @brief Causes the backend to handle any file descriptors that are ready
            to be read from/written to.
         
         @param read The set of file descriptors that have pending reads.
         @param write The set of file descriptors that have pending writes.
      */
      virtual void process(fd_set& read, fd_set& write) = 0;

      /**
         @brief Allow the backend to clean up any resources associated with res.
         @param res The result that the user is done with.
      */
      virtual void freeResult(ExternalDnsRawResult res) = 0;

      /**
         @brief Allow the backend to clean up any resources associated with res.
         @param res The result that the user is done with.
      */
      virtual void freeResult(ExternalDnsHostResult res) = 0;

      /**
         @brief Returns a text description of errorCode.
         @param errorCode An error code returned by init()
         @return A c string containing the description of the error code. Caller
            is responsible for cleaning up the memory using delete [].
         @note The implementation must allocate the returned string with new, 
            NOT malloc or any other allocator.
      */
      virtual char* errorMessage(long errorCode) = 0;
      
      virtual ~ExternalDns()  {}

      /**
         @brief Begin a DNS lookup, which will result in a callback when done.

         @param target The name to resolve.
         @param type The query type to carry out. (the standard resource record 
            type number)
         @param handler The handler that should get a callback when the query 
            completes (see ExternalDnsHandler::handleDnsRaw())
         @param userData Pointer to some arbitrary data that should be passed 
         back to handler in the ExternalDnsRawResult.
      */
      virtual void lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData) = 0;

      virtual bool hostFileLookup(const char* target, in_addr &addr) = 0;
      virtual bool hostFileLookupLookupOnlyMode() = 0;
};
 
class ExternalDnsResult : public AsyncResult
{
   public:
      ExternalDnsResult(long errorCode, void* uData) : AsyncResult(errorCode) , userData(uData) {}
      ExternalDnsResult(void* uData) : userData(uData) {}
      void* userData;
};

//should this be nested?
class ExternalDnsRawResult : public ExternalDnsResult
{
   public:
      ExternalDnsRawResult(unsigned char* buf, int len, void* uData) : 
         ExternalDnsResult(uData),

         abuf(buf),
         alen(len) 
      {}
      ExternalDnsRawResult(long errorCode, unsigned char* buf, int len, void* uData) : 
         ExternalDnsResult(errorCode, uData),
         abuf(buf),
         alen(len)
         {}
         
      unsigned char* abuf;
      int alen;
};

class ExternalDnsHostResult : public ExternalDnsResult
{
   public:
      ExternalDnsHostResult(hostent* h, void* uData) :
         ExternalDnsResult(uData), 
         host(h)
      {}
      ExternalDnsHostResult(long errorCode, void* uData) : ExternalDnsResult(errorCode, uData) {}
         
      hostent* host;
};

class ExternalDnsHandler
{
   public:
      //underscores are against convention, but pretty impossible to read
      //otherwise. ?dcm? -- results stack or heap? 
      //the free routines can be dealt w/ iheritence instead if pointers are used
      //virtual void handle_NAPTR(ExternalDnsRawResult res) = 0;
      //virtual void handle_SRV(ExternalDnsRawResult res) = 0;
      //virtual void handle_AAAA(ExternalDnsRawResult res) = 0;
      //virtual void handle_host(ExternalDnsHostResult res) = 0;

      // new version
      virtual ~ExternalDnsHandler() {}
      virtual void handleDnsRaw(ExternalDnsRawResult res) = 0;
};

}

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
