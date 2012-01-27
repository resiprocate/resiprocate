/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifndef UDP_HXX
#define UDP_HXX


/** 
    @file 
    @brief UDP utilities for STUN and TURN.
*/



#ifdef __MACH__
#  ifdef __APPLE__
#     include <sys/socket.h>
#     if !defined(_BSD_SOCKLEN_T_) && !defined(_SOCKLEN_T)
         typedef int socklen_t;
#     endif
#  else
      typedef int socklen_t;
#  endif
#endif

#include "rutil/Socket.hxx"

/**
   Open a UDP socket to receive on the given port - if port is 0, pick
   a port, if interfaceIp!=0 then use ONLY the interface specified
   instead of all of them.

   @param port the UDP port number to bind to.

   @param interfaceIp The 32 bits that will be cast as struct
   sin_addr.s_addr

   @return The Socket bound to the specified address and port.

   @retval INVALID_SOCKET if the address and port could not be bound.
   
*/
resip::Socket
openPort( unsigned short port, unsigned int interfaceIp,
          bool verbose);


/**
   Receive a UDP message.

   @param fd The socket that will be read.

   @param buf The output parameter that will hold the bytes that were
   read from the fd.

   @param len The output parameter that will hold the number of bytes
   read. The value passed in is interpreted as the size of the buf.

   @param srcIp The output parameter holding the source IP address of
   the received UDP datagram.

   @param srcPort The output parameter holding the source port number
   of the received UDP datagram.

   @param verbose If true, verbose logs are generated.
*/
bool 
getMessage( resip::Socket fd, char* buf, int* len,
            UInt32* srcIp, unsigned short* srcPort,
            bool verbose);


/**
   Send a UDP message.

   @param fd The socket that will be written to.

   @param buf Holds the data that will be written.

   @param len The number of bytes in buf that will be written.

   @param dstIp The destination IP address for the UDP datagram.

   @param dstPort The destination UDP port for the UDP datagram.

   @param verbose If true, verbose logs are generated.
*/
bool 
sendMessage( resip::Socket fd, char* msg, int len, 
             unsigned int dstIp, unsigned short dstPort,
             bool verbose);

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
 */

// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End:

#endif
