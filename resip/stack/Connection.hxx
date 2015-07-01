#ifndef RESIP_Connection_hxx
#define RESIP_Connection_hxx

#include <list>

#include "resip/stack/ConnectionBase.hxx"
//#include "rutil/Fifo.hxx"
#include "rutil/Socket.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "rutil/IntrusiveListElement.hxx"

namespace resip
{

class Message;
class TlsConnection;
class ConnectionManager;
class Connection;
class Compression;

/// three intrusive list types for in-place reference
typedef IntrusiveListElement<Connection*> ConnectionLruList;
typedef IntrusiveListElement1<Connection*> ConnectionReadList;
typedef IntrusiveListElement2<Connection*> ConnectionWriteList;
typedef IntrusiveListElement3<Connection*> FlowTimerLruList;

/** Connection implements, via sockets, ConnectionBase for managed
    connections. Connections are managed for approximate fairness and least
    recently used garbage collection.
    Connection inherits three different instantiations of intrusive lists.
*/
class Connection : public ConnectionBase, 
                   public ConnectionLruList, 
                   public ConnectionReadList, 
                   public ConnectionWriteList, 
                   public FlowTimerLruList, 
                   public FdPollItemIf
{
      friend class ConnectionManager;
      friend EncodeStream& operator<<(EncodeStream& strm, const resip::Connection& c);

   public:
      Connection(Transport* transport,const Tuple& who, Socket socket, Compression &compression);
      virtual ~Connection();
      
      /*!
         @note Right now, Connection is assumed to be a TCP connection.
               This means that there is a 1-1 correspondence between
               FD and Connection. If this changes down the line (say, if
               we use this class to do SCTP connections), we can just remove
               this function.
      */
      Socket getSocket() const {return mWho.mFlowKey;}

      /// always true -- always add to fdset as read ready
      virtual bool hasDataToRead();
      /// has valid connection
      virtual bool isGood();
      virtual bool checkConnectionTimedout();
      virtual bool isWritable();
      virtual bool transportWrite(){return false;}

      /// queue data to write and add this to writable list
      void requestWrite(SendData* sendData);

      /// send some or all of a queued data; remove from writable if completely written
      int performWrite();

      /** Call performWrite() repeatedly, until either the send queue is 
            exhausted, the write() call fails (probably because the fd is no 
            longer ready to write), or a set number of writes has been 
            performed. 
         @param max The maximum number of writes to perform. 0 indicates that
            there is no limit.
         @return false iff this connection has deleted itself.
      */
      bool performWrites(unsigned int max=0);

      /// ensure that we are on the writeable list if required
      void ensureWritable();

      /** move data from the connection to the buffer; move this to front of
          least recently used list. when the message is complete,
          it is delivered via mTransport->pushRxMsgUp()
          which generally puts it on a fifo */
      int read();

      /** Call read() repeatedly, until an error occurs (because the fd is not 
            ready to read, most likely).
         @param max The maximum number of reads to perform. 0 indicates that
            there is no limit.
         @return false iff this connection has deleted itself.
      */
      bool performReads(unsigned int max=0);

      /// Ensures this connection is in the FlowTimer LRU list in the connection manager
      void enableFlowTimer();
      bool isFlowTimerEnabled() { return mFlowTimerEnabled; }

      bool mFirstWriteAfterConnectedPending;
      static volatile bool mEnablePostConnectSocketFuncCall;
      static void setEnablePostConnectSocketFuncCall(bool enabled = true) { mEnablePostConnectSocketFuncCall = enabled; }

   protected:
      /// pure virtual, but need concrete Connection for book-ends of lists
      virtual int read(char* /* buffer */, const int /* count */) { return 0; }
      /// pure virtual, but need concrete Connection for book-ends of lists
      virtual int write(const char* /* buffer */, const int /* count */) { return 0; }
      virtual void onDoubleCRLF();
      virtual void onSingleCRLF();

      /* callback method of FdPollItemIf */
      virtual void processPollEvent(FdPollEventMask mask);

   private:
      ConnectionManager& getConnectionManager() const;
      void removeFrontOutstandingSend();
      bool mInWritable;
      bool mFlowTimerEnabled;
      FdPollItemHandle mPollItemHandle;
      
      /// no default c'tor
      Connection();

      /// no value semantics
      Connection(const Connection&);
      Connection& operator=(const Connection&);
};

EncodeStream& 
operator<<(EncodeStream& strm, const resip::Connection& c);

}

#endif
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
