#if !defined(RESIP_CONNECTION_HXX)
#define RESIP_CONNECTION_HXX 

#include <list>

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Transport.hxx"
#ifndef NEW_MSG_HEADER_SCANNER
#include "resiprocate/Preparse.hxx"
#else
#include "resiprocate/MsgHeaderScanner.hxx"
#endif
#include "resiprocate/os/IntrusiveListElement.hxx"

namespace resip
{

class Message;
class TlsConnection;

class Connection : public IntrusiveListElement<Connection*, 1>, 
                   public IntrusiveListElement<Connection*, 2>,
                   public IntrusiveListElement<Connection*, 3>
{
      typedef IntrusiveListElement<Connection*, 1> lruList;
      typedef IntrusiveListElement<Connection*, 2> readList;
      typedef IntrusiveListElement<Connection*, 3> writeList;

      friend class ConnectionManager;
      friend std::ostream& operator<<(std::ostream& strm, const resip::Connection& c);

   public:
      Connection(const Tuple& who, Socket socket);

      ConnectionId getId() const;
      Socket getSocket() const {return mSocket;}

      //bool hasDataToWrite() const;
      void requestWrite(SendData* sendData);

      void performRead(int bytesRead, Fifo<Message>& fifo);
      void performWrite();

      enum { ChunkSize = 2048 }; //!dcm! -- bad size, perhaps 2048-4096?
      
      // pure virtual, but need concrete Connection
      virtual int read(char* buffer, int count) { return 0; }
      virtual int write(const char* buffer, int count) { return 0; }
      
   protected:
      Connection();
      virtual ~Connection();

      Socket mSocket;

   private:
      ConnectionManager& getConnectionManager() const;
      std::pair<char*, size_t> getWriteBuffer();

      void remove(); // called by ConnectionManager

      // no value semantics
      Connection(const Connection&);
      Connection& operator=(const Connection&);

      Tuple mWho;
      // position in current message being sent
      Data::size_type mSendPos;
      std::list<SendData*> mOutstandingSends; // !jacob! intrusive queue?

      SipMessage* mMessage;
      char* mBuffer;
      size_t mBufferPos;
      size_t mBufferSize;

      enum State
      {
         NewMessage = 0,
         ReadingHeaders,
         PartialBody,
         MAX
      };

      static char connectionStates[MAX][32];

      UInt64 mLastUsed;
            
      State mState;
#ifndef NEW_MSG_HEADER_SCANNER
      Preparse mPreparse;
#else
      MsgHeaderScanner mMsgHeaderScanner;
#endif

      friend class TcpBaseTransport;
};

std::ostream& 
operator<<(std::ostream& strm, const resip::Connection& c);

}

#endif
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
