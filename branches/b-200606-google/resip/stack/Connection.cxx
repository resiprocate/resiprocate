#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/TcpBaseTransport.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

Connection::Connection()
   : mSocket(INVALID_SOCKET)
{
}

Connection::Connection(const Tuple& who, Socket socket)
   : ConnectionBase(who),
     mSocket(socket)
{
   getConnectionManager().addConnection(this);
}

Connection::~Connection()
{
   if (mSocket != INVALID_SOCKET) // bogus Connections
   {
      closeSocket(mSocket);
      getConnectionManager().removeConnection(this);
   }
}

ConnectionId
Connection::getId() const
{
   return mWho.connectionId;
}

void
Connection::requestWrite(SendData* sendData)
{
   assert(mWho.transport);
   if (mOutstandingSends.empty())
   {
      getConnectionManager().addToWritable(this);
   }
   mOutstandingSends.push_back(sendData);
}

void
Connection::performWrite()
{
   assert(!mOutstandingSends.empty());
   const Data& data = mOutstandingSends.front()->data;
//   DebugLog (<< "Sending " << data.size() - mSendPos << " bytes");
   int nBytes = write(data.data() + mSendPos,data.size() - mSendPos);

   if (nBytes < 0)
   {
      //fail(data.transactionId);
      InfoLog(<< "Write failed on socket: " << this->getSocket() << ", closing connection");
      delete this;
   }
   else
   {
      // Safe because of the conditional above ( < 0 ).
      Data::size_type bytesWritten = static_cast<Data::size_type>(nBytes);
      mSendPos += bytesWritten;
      if (mSendPos == data.size())
      {
         mSendPos = 0;
         delete mOutstandingSends.front();
         mOutstandingSends.pop_front();

         if (mOutstandingSends.empty())
         {
            getConnectionManager().removeFromWritable();
         }
      }
   }
}
            
ConnectionManager&
Connection::getConnectionManager() const
{
   assert(mWho.transport);
   TcpBaseTransport* transport = static_cast<TcpBaseTransport*>(mWho.transport);
   
   return transport->getConnectionManager();
}
            
std::ostream& 
resip::operator<<(std::ostream& strm, const resip::Connection& c)
{
   strm << "CONN: " << &c << " " << int(c.getSocket()) << " " << c.mWho;
   return strm;
}

Transport* 
Connection::transport()
{
   return mWho.transport;
}

int
Connection::read(Fifo<TransactionMessage>& fifo)
{
   std::pair<char*, size_t> writePair = getWriteBuffer();
   size_t bytesToRead = resipMin(writePair.second, 
                                 static_cast<size_t>(Connection::ChunkSize));
         
   assert(bytesToRead > 0);
   int bytesRead = read(writePair.first, bytesToRead);
   if (bytesRead <= 0)
   {
      return bytesRead;
   }  
   getConnectionManager().touch(this);
   preparseNewBytes(bytesRead, fifo); //.dcm. may delete this   
   return bytesRead;
}

bool 
Connection::hasDataToRead()
{
   return true;
}

bool 
Connection::isGood()
{
   return true;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000
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

