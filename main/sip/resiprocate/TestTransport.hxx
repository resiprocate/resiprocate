#if !defined(TESTTRANSPORT_HXX)
#define TESTTRANSPORT_HXX

#include "sip2/sipstack/Transport.hxx"
#include "sip2/sipstack/Message.hxx"
#include "sip2/sipstack/Circular.hxx"

namespace Vocal2
{


class TestReliableTransport : public Transport
{
   public:
      TestReliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo);
      ~TestReliableTransport();
    
      void process(FdSet& fdset);
      void buildFdSet(FdSet& fdset);
    
      bool isReliable() const;
      Type transport() const;

};

class TestUnreliableTransport : public Transport
{
   public:
      TestUnreliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo);
      ~TestUnreliableTransport();
    
      void process(FdSet& fdset);
      void buildFdSet(FdSet& fdset);
    
      bool isReliable() const;
      Type transport() const;

};

// singleton classes for input and output buffers

typedef Circular TestBufType;


class TestInBuffer
{
   private:
      static TestInBuffer* mInstance;

      TestBufType mCbuf;

   protected:
      TestInBuffer();

   public:
      static TestInBuffer& instance();
    
      TestBufType& getBuf() { return mCbuf; }
};

class TestOutBuffer
{
   private:
      static TestOutBuffer* mInstance;

      TestBufType mCbuf;

   protected:
      TestOutBuffer();

   public:
      static TestOutBuffer& instance();
    
      TestBufType& getBuf() { return mCbuf; }
};


void sendToWire(const Data&);  // put data on wire to go to stack
void getFromWire(Data&);       // get data off wire that has come from stack

void stackSendToWire(const Data&);  // put data from stack onto wire
void stackGetFromWire(Data&);       // get data from wire to go to stack
  
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
