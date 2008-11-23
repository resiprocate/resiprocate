#ifndef __P2P_CANDIDATE_HXX
#define __P2P_CANDIDATE_HXX 1

#include "rutil/GenericIPAddress.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Logger.hxx"

#include "p2p/P2PSubsystem.hxx"
namespace p2p
{

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

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
               << ntohs(address.v4Address.sin_port) << " typ host";
         }
         mValue = tmp;
      }

      Candidate(const resip::Data &iceline)
      {
         resip::Data transport;
         resip::Data ip("127.0.0.1"); // WRONG WRONG WRONG
         resip::Data port;
         const char *anchor;
         
         DebugLog(<< "Constructing Candidate from iceline " << iceline);
         resip::ParseBuffer pb(iceline);
         
         // At beginning
         pb.skipToChar(' ');  // At end of foundation
         pb.skipWhitespace(); // Skip the whitespace after foundation
         
         // Beginning of component ID
         pb.skipToChar(' '); // End of component ID

         anchor = pb.skipWhitespace(); // Beginning of transport
         pb.skipToChar(' '); // End of transport
         pb.data(transport, anchor);
         DebugLog(<< "transport is " << transport);
         
         pb.skipWhitespace();  // Beginning of priority
         pb.skipToChar(' '); // End of priority

         anchor = pb.skipWhitespace(); // Beginning of IP
         pb.skipToChar(' '); // End of IP
         // pb.data(ip, anchor); WRONG WRONG WRONG
         DebugLog(<< "IP is " << ip);

         anchor=pb.skipWhitespace(); // Beginning of port
         pb.skipToChar(' '); // End of port
         pb.data(port, anchor);
         DebugLog(<< "port is " << port);

         mTransportType = resip::getTransportTypeFromName(transport.c_str());

         mAddress.v4Address.sin_family = AF_INET;
         mAddress.v4Address.sin_port = htons(port.convertInt());
         memset(mAddress.v4Address.sin_zero, 0,
                sizeof(mAddress.v4Address.sin_zero)) ;

         resip::DnsUtil::inet_pton(ip, mAddress.v4Address.sin_addr);

			mValue = iceline;
      }

      const resip::Data &getIceString() const { return mValue; }

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
