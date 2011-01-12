#ifndef TURNUDPSOCKET_HXX
#define TURNUDPSOCKET_HXX

#include <boost/asio.hpp>

#include "TurnSocket.hxx"

namespace reTurn {

   class TurnUdpSocket : public TurnSocket
{
public:
   explicit TurnUdpSocket(const boost::asio::ip::address& address, unsigned short port);  

   virtual unsigned int getSocketDescriptor() { return mSocket.native(); }

   virtual  boost::system::error_code connect(const std::string& address, unsigned short port);

protected:
   virtual boost::system::error_code rawWrite(const char* buffer, unsigned int size);
   virtual boost::system::error_code rawWrite(const std::vector<boost::asio::const_buffer>& buffers);
   virtual boost::system::error_code rawRead(unsigned int timeout, unsigned int* bytesRead, boost::asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   virtual void cancelSocket();

private:
   boost::asio::ip::udp::socket mSocket;

   // Remote binding info
   boost::asio::ip::udp::endpoint mRemoteEndpoint;
   boost::asio::ip::udp::endpoint mSenderEndpoint;  // for read operations
};

} 

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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

 ==================================================================== */
