#include <memory>
#include <iostream>
#include <iterator>

#include <sipstack/Helper.hxx>
#include <sipstack/Preparse.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/TestTransport.hxx>
#include <util/Data.hxx>
#include <util/DataStream.hxx>
#include <util/Logger.hxx>
#include <util/Socket.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP



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

   
  // how do we know that buffer won't get deleted on us !jf!
  if (mTxFifo.messageAvailable())
    {
      std::auto_ptr<SendData> data = std::auto_ptr<SendData>(mTxFifo.getNext());
      DebugLog (<< "Sending message on TestReliable");

      // cerr << data->data.c_str();

      // write message into output buffer singleton
      for (int i=0; i<data->data.size(); i++)
	{
	  TestOutBuffer::instance().getBuf().push_back(data->data.c_str()[i]);
	}


   }

  if ( ! TestInBuffer::instance().getBuf().empty())
    {
      TestBufType& inBuf = TestInBuffer::instance().getBuf();
      int len = inBuf.size(); 
      char* buffer = new char(len);
      
      int i = 0;
      for ( TestBufType::const_iterator iter = inBuf.begin();
	    iter != inBuf.end();
	    iter++, i++ )
	{
	  buffer[i] = *iter;
	}

      SipMessage* message = Helper::makeMessage(buffer);
      delete [] buffer;
      
      // set the received from information into the received= parameter in the
      // via
      // sockaddr_in from;
      // message->setSource(from);

      DebugLog (<< "adding new SipMessage to state machine's Fifo: " << message->brief());
      
      // set the received= and rport= parameters in the message if necessary !jf!
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
  return TestReliable; 
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
