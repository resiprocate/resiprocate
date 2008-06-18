#ifndef __P2P_CANDIDATE_HXX
#define __P2P_CANDIDATE_HXX 1

#include "rutil/GenericIPAddress.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"

namespace p2p
{

//candidate:1 1 UDP 2130706431 10.0.1.1 8998 typ host

class Candidate
{
   public:

      Candidate(resip::TransportType type, resip::GenericIPAddress& address)
        : mTransportType(type), mAddress(address)
      {
         resip::Data tmp;
         {
            resip::DataStream ds(tmp);

            ds << "candidate:1 1 " 
               << resip::getTransportNameFromType(type) << " "
               << rand() << " "
               << resip::DnsUtil::inet_ntop(address.v4Address.sin_addr) << " "
               << address.v4Address.sin_port << " typ host";
         }
         mValue = tmp;
      }

      Candidate(const resip::Data &iceline)
      {
         resip::Data transport;
         resip::Data ip;
         resip::Data port;
         const char *anchor;

         resip::ParseBuffer pb(iceline);

         pb.skipToChar(' ');
         pb.skipToChar(' ');

         anchor = pb.skipChar(' ');
         pb.skipToChar(' ');
         pb.data(transport, anchor);

         pb.skipToChar(' ');
         anchor = pb.skipChar(' ');
         pb.skipToChar(' ');
         pb.data(ip, anchor);

         anchor = pb.skipChar(' ');
         pb.skipToChar(' ');
         pb.data(port, anchor);

         mTransportType = resip::getTransportTypeFromName(transport.c_str());

         mAddress.v4Address.sin_family = AF_INET;
         mAddress.v4Address.sin_port = port.convertInt();
         memset(mAddress.v4Address.sin_zero, 0,
                sizeof(mAddress.v4Address.sin_zero)) ;

         resip::DnsUtil::inet_pton(ip, mAddress.v4Address.sin_addr);
      }

      const resip::TransportType    &getTransportType() const 
         { return mTransportType; }

      const resip::GenericIPAddress &getAddress() const
         { return mAddress; }

   private:
      resip::Data mValue;
      resip::TransportType    mTransportType;
      resip::GenericIPAddress mAddress;
      // TODO add ice specific members
};

}

#endif

/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
