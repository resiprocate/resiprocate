#include <iostream>
#include <iterator>

#include "util/Data.hxx"
#include "util/DataStream.hxx"
#include "util/Socket.hxx"
#include "util/Logger.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Helper.hxx"

#include "sipstack/TestTransport.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

void 
Vocal2::sendToWire(const Data& data)  // put data on wire to go to stack
{
   // write message into input buffer singleton
   for (int i=0; i<data.size(); i++)
   {
      TestInBuffer::instance().getBuf().push_back(data.c_str()[i]);
   }

}


void 
Vocal2::getFromWire(Data& data)       // get data off wire that has come from stack
{
   TestBufType& cbuf = TestOutBuffer::instance().getBuf();
   int len = cbuf.size();

   for (int i=0; i<len; i++ )
   {
      data += cbuf.front();
      cbuf.pop_front();
   }
  
}

void 
Vocal2::stackSendToWire(const Data& data)  // put data from stack onto wire
{
   // write message into input buffer singleton
   for (int i=0; i<data.size(); i++)
   {
      TestOutBuffer::instance().getBuf().push_back(data.c_str()[i]);
   }

}


void 
Vocal2::stackGetFromWire(Data& data)       // get data from wire to go to stack
{
   TestBufType& cbuf = TestInBuffer::instance().getBuf();
   int len = cbuf.size();

   for (int i=0; i<len; i++ )
   {
      data += cbuf.front();
      cbuf.pop_front();
   }
  
}


TestReliableTransport::TestReliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo)
   : Transport(sendHost, portNum, nic, rxFifo)
{
}

TestReliableTransport::~TestReliableTransport()
{
}
    


void 
TestReliableTransport::process(fd_set* fdSet)
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   
   if (mTxFifo.messageAvailable())
   {
      std::auto_ptr<SendData> data = std::auto_ptr<SendData>(mTxFifo.getNext());
      DebugLog (<< "Sending message on TestReliable");

      stackSendToWire(data->data.data());
   }


   if ( !  TestInBuffer::instance().getBuf().empty() )
   {
      Data data;

      stackGetFromWire(data);

      SipMessage* message = Helper::makeMessage(data);
      
      mStateMachineFifo.add(message);
   }
}

void 
TestReliableTransport::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
   // nothing to do here
}
    
bool 
TestReliableTransport::isReliable() const
{ 
   return true; 
}

Transport::Type 
TestReliableTransport::transport() const 
{ 
   return Transport::TCP; 
}



TestUnreliableTransport::TestUnreliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo)
   : Transport(sendHost, portNum, nic, rxFifo)
{
}

TestUnreliableTransport::~TestUnreliableTransport()
{
}
    


void 
TestUnreliableTransport::process(fd_set* fdSet)
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   
   if (mTxFifo.messageAvailable())
   {
      std::auto_ptr<SendData> data = std::auto_ptr<SendData>(mTxFifo.getNext());
      DebugLog (<< "Sending message on TestUnreliable");

      stackSendToWire(data->data.data());
   }


   if ( !  TestInBuffer::instance().getBuf().empty() )
   {
      Data data;

      stackGetFromWire(data);

      SipMessage* message = Helper::makeMessage(data);
      
      mStateMachineFifo.add(message);
   }
}

void 
TestUnreliableTransport::buildFdSet( fd_set* fdSet, int* fdSetSize )
{
   // nothing to do here
}
    
bool 
TestUnreliableTransport::isReliable() const
{ 
   return false; 
}

Transport::Type 
TestUnreliableTransport::transport() const 
{ 
   return Transport::UDP;
}





TestInBuffer* TestInBuffer::mInstance = 0;
TestOutBuffer* TestOutBuffer::mInstance = 0;

TestInBuffer::TestInBuffer() :
   mCbuf(100000)
{
}


TestInBuffer&
TestInBuffer::instance()
{
   if (mInstance == 0)
      mInstance = new TestInBuffer();

   return *mInstance;
}


TestOutBuffer::TestOutBuffer() :
   mCbuf(100000)
{
}


TestOutBuffer&
TestOutBuffer::instance()
{
   if (mInstance == 0)
      mInstance = new TestOutBuffer();

   return *mInstance;
}
