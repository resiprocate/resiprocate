#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/TcpConnection.hxx"
#include "resip/stack/Tuple.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TcpConnection::TcpConnection(Transport* transport,const Tuple& who, Socket fd,
                             Compression &compression, bool isServer)
  : Connection(transport,who, fd, compression, isServer)
{
   DebugLog (<< "Creating TCP connection " << who << " on " << fd);
}

int 
TcpConnection::read( char* buf, int count )
{
   resip_assert(buf);
   resip_assert(count > 0);
   
#if defined(WIN32)
   int bytesRead = ::recv(getSocket(), buf, count, 0);
#else
   int bytesRead = ::read(getSocket(), buf, count);
#endif

   if (bytesRead == SOCKET_ERROR)
   {
      int e = getErrno();
      switch (e)
      {
         case EAGAIN:
#if EAGAIN != EWOULDBLOCK
         case EWOULDBLOCK:  // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
#endif
            StackLog (<< "No data ready to read");
            return 0;
         case EINTR:
            DebugLog (<< "The call was interrupted by a signal before any data was read.");
            return 0;            
            break;
         case EIO:
            InfoLog (<< "I/O error");
            break;
         case EBADF:
            InfoLog (<< "fd is not a valid file descriptor or is not open for reading.");
            break;
         case EINVAL:
            InfoLog (<< "fd is attached to an object which is unsuitable for reading.");
            break;
         case EFAULT:
            ErrLog (<< "buf is outside your accessible address space.");
            break;
         default:
            ErrLog (<< "Some other error, code = " << e);
            break;
      }

      InfoLog (<< "Failed read on " << getSocket() << " " << strerror(e));
      Transport::error(e);
      setFailureReason(TransportFailure::ConnectionException, e+2000);
      return -1;
   }
   else if (bytesRead == 0)
   {
      InfoLog (<< "Connection closed by remote " << *this);
      return -1;
   }

   return bytesRead;
}


int 
TcpConnection::write( const char* buf, const int count )
{
   //DebugLog (<< "Writing " << buf);   // Note:  this can end up writing garbage to the logs following the message for non-null terminated buffers

   resip_assert(buf);
   resip_assert(count > 0);

#if defined(WIN32)
   int bytesWritten = ::send(getSocket(), buf, count, 0);
#else
   int bytesWritten = ::write(getSocket(), buf, count);
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      //setFailureReason(TransportFailure::ConnectionException, e+1000);
      if (e == EAGAIN || e == EWOULDBLOCK) // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
      {
          // TCP buffers are backed up - we couldn't write anything - but we shouldn't treat this an error - return we wrote 0 bytes
          return 0;
      }
      InfoLog (<< "Failed write on " << getSocket() << " " << strerror(e));
      Transport::error(e);
      return -1;
   }
   
   return bytesWritten;
}

bool 
TcpConnection::hasDataToRead()
{
   return false;
}

bool 
TcpConnection::isGood()
{
   return true;
}

bool 
TcpConnection::isWritable()
{
   return true;
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
 * vi: set shiftwidth=3 expandtab:
 */
