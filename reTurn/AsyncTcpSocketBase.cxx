#include <boost/bind.hpp>

#include "AsyncTcpSocketBase.hxx"
#include "AsyncSocketBaseHandler.hxx"
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"
#include "QosSocketManager.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

using namespace std;

namespace reTurn {

AsyncTcpSocketBase::AsyncTcpSocketBase(boost::asio::io_service& ioService) 
 : AsyncSocketBase(ioService),
   mSocket(ioService), 
   mResolver(ioService)
{
}

AsyncTcpSocketBase::~AsyncTcpSocketBase() 
{
}

unsigned int 
AsyncTcpSocketBase::getSocketDescriptor() 
{ 
   return mSocket.native(); 
}

boost::system::error_code 
AsyncTcpSocketBase::bind(const boost::asio::ip::address& address, unsigned short port)
{
   boost::system::error_code errorCode;
   mSocket.open(address.is_v6() ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), errorCode);
   if(!errorCode)
   {
      mSocket.set_option(boost::asio::ip::tcp::no_delay(true)); // ?slg? do we want this?
      mSocket.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
      mSocket.bind(boost::asio::ip::tcp::endpoint(address, port), errorCode);
   }   
   return errorCode;
}

void 
AsyncTcpSocketBase::connect(const std::string& address, unsigned short port)
{
   // Start an asynchronous resolve to translate the address
   // into a list of endpoints.
   resip::Data service(port);
#ifdef USE_IPV6
   boost::asio::ip::tcp::resolver::query query(address, service.c_str());   
#else
   boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), address, service.c_str());   
#endif
   mResolver.async_resolve(query,
        boost::bind(&AsyncSocketBase::handleTcpResolve, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::iterator));
}

void 
AsyncTcpSocketBase::handleTcpResolve(const boost::system::error_code& ec,
                                  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      //boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      mSocket.async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            boost::asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void 
AsyncTcpSocketBase::handleConnect(const boost::system::error_code& ec,
                                  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
   if (!ec)
   {
      // The connection was successful.
      mConnected = true;
      mConnectedAddress = endpoint_iterator->endpoint().address();
      mConnectedPort = endpoint_iterator->endpoint().port();

      onConnectSuccess();
   }
   else if (++endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
   {
      // The connection failed. Try the next endpoint in the list.
      mSocket.close();
      mSocket.async_connect(endpoint_iterator->endpoint(),
                            boost::bind(&AsyncSocketBase::handleConnect, shared_from_this(),
                            boost::asio::placeholders::error, endpoint_iterator));
   }
   else
   {
      onConnectFailure(ec);
   }
}

void
AsyncTcpSocketBase::setConnectedAddressAndPort()
{
   mConnectedAddress = mSocket.remote_endpoint().address();
   mConnectedPort = mSocket.remote_endpoint().port();
}

const boost::asio::ip::address 
AsyncTcpSocketBase::getSenderEndpointAddress() 
{ 
   return mConnectedAddress; 
}

unsigned short 
AsyncTcpSocketBase::getSenderEndpointPort() 
{ 
   return mConnectedPort; 
}

void 
AsyncTcpSocketBase::transportSend(const StunTuple& destination, std::vector<boost::asio::const_buffer>& buffers)
{
   // Note: destination is ignored for TCP
   boost::asio::async_write(mSocket, buffers, 
                     boost::bind(&AsyncTcpSocketBase::handleSend, shared_from_this(), boost::asio::placeholders::error));
}

void 
AsyncTcpSocketBase::transportReceive()
{
   mSocket.async_read_some(boost::asio::buffer((void*)mReceiveBuffer->data(), RECEIVE_BUFFER_SIZE),
                           boost::bind(&AsyncTcpSocketBase::handleReceive, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void 
AsyncTcpSocketBase::transportFramedReceive()
{
   boost::asio::async_read(mSocket, boost::asio::buffer((void*)mReceiveBuffer->data(), 4),
                    boost::bind(&AsyncSocketBase::handleReadHeader, shared_from_this(), boost::asio::placeholders::error));
}

void 
AsyncTcpSocketBase::handleReadHeader(const boost::system::error_code& e)
{
   if (!e)
   {
      /*      
      std::cout << "Read header from tcp socket: " << std::endl;
      for(unsigned int i = 0; i < 4; i++)
      {
         std::cout << (char)(*mReceiveBuffer)[i] << "(" << (int)(*mReceiveBuffer)[i] << ") ";
      }
      std::cout << std::endl;
      */

      // Note:  For both StunMessages and ChannelData messages the length in bytes 3 and 4
      UInt16 dataLen;
      memcpy(&dataLen, &(*mReceiveBuffer)[2], 2);
      dataLen = ntohs(dataLen);

      if(((*mReceiveBuffer)[0] & 0xC0) == 0)  // If first 2 bits are 00 then this is a stun message
      {
         dataLen += 16;  // There are 20 bytes in total in the header, and we have already read 4 - read the rest of the header + the body
      }
      if(dataLen+4 < RECEIVE_BUFFER_SIZE)
      {
         boost::asio::async_read(mSocket, boost::asio::buffer(&(*mReceiveBuffer)[4], dataLen),
                          boost::bind(&AsyncTcpSocketBase::handleReceive, shared_from_this(), boost::asio::placeholders::error, dataLen+4));
      }
      else
      {
         WarningLog(<< "Receive buffer (" << RECEIVE_BUFFER_SIZE << ") is not large enough to accomdate incoming framed data (" << dataLen+4 << ") closing connection.");
         close();
      }
   }
   else if (e != boost::asio::error::operation_aborted)
   {
      if(e != boost::asio::error::eof && e != boost::asio::error::connection_reset)
      {
         WarningLog(<< "Read header error: " << e.value() << "-" << e.message());
      }
      close();
   }
}

void 
AsyncTcpSocketBase::transportClose()
{
   QosSocketManager::SocketClose(mSocket.native());

   mSocket.close();
}

bool 
AsyncTcpSocketBase::setDSCP(boost::uint32_t ulInDSCPValue)
{
   return QosSocketManager::SocketSetDSCP(mSocket.native(), ulInDSCPValue, false);
}

bool 
AsyncTcpSocketBase::setServiceType(
   const boost::asio::ip::udp::endpoint &tInDestinationIPAddress,
   EQOSServiceTypes eInServiceType,
   boost::uint32_t ulInBandwidthInBitsPerSecond
)
{
   return QosSocketManager::SocketSetServiceType(mSocket.native(), 
      tInDestinationIPAddress, eInServiceType, ulInBandwidthInBitsPerSecond, false);
}

}


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

