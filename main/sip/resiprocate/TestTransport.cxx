#include <iostream>
#include <iterator>
#include <memory>

#include "sip2/util/Data.hxx"
#include "sip2/util/DataStream.hxx"
#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/Helper.hxx"

#include "sip2/sipstack/TestTransport.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

void 
Vocal2::sendToWire(const Data& data)  // put data on wire to go to stack
{
   TestInBuffer::instance().getBuf().add(new Data(data));
}


void 
Vocal2::getFromWire(Data& data)       // get data off wire that has come from stack
{
   data = *TestOutBuffer::instance().getBuf().getNext();
}

void 
Vocal2::stackSendToWire(const Data& data)  // put data from stack onto wire
{
   TestOutBuffer::instance().getBuf().add(new Data(data));
}


void 
Vocal2::stackGetFromWire(Data& data)       // get data from wire to go to stack
{
   data = *TestInBuffer::instance().getBuf().getNext();
}


TestReliableTransport::TestReliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo)
   : Transport(sendHost, portNum, nic, rxFifo)
{
}

TestReliableTransport::~TestReliableTransport()
{
}
    


void 
TestReliableTransport::process(FdSet& fdSet)
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


   if ( TestInBuffer::instance().getBuf().messageAvailable() )
   {
      Data data;

      stackGetFromWire(data);

      SipMessage* message = Helper::makeMessage(data);
      
      mStateMachineFifo.add(message);
   }
}

void 
TestReliableTransport::buildFdSet( FdSet& fdset )
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
   return Transport::TestReliable; 
}



TestUnreliableTransport::TestUnreliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo)
   : Transport(sendHost, portNum, nic, rxFifo)
{
}

TestUnreliableTransport::~TestUnreliableTransport()
{
}
    


void 
TestUnreliableTransport::process(FdSet& fdset)
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


   if ( TestInBuffer::instance().getBuf().messageAvailable() )
   {
      Data data;

      stackGetFromWire(data);

      SipMessage* message = Helper::makeMessage(data);
      
      mStateMachineFifo.add(message);
   }
}

void 
TestUnreliableTransport::buildFdSet( FdSet& fdset )
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
   return Transport::TestUnreliable;
}





TestInBuffer* TestInBuffer::mInstance = 0;
TestOutBuffer* TestOutBuffer::mInstance = 0;

TestInBuffer::TestInBuffer()
{
}


TestInBuffer&
TestInBuffer::instance()
{
   if (mInstance == 0)
      mInstance = new TestInBuffer();

   return *mInstance;
}


TestOutBuffer::TestOutBuffer()
{
}


TestOutBuffer&
TestOutBuffer::instance()
{
   if (mInstance == 0)
      mInstance = new TestOutBuffer();

   return *mInstance;
}
