#ifdef WIN32

#include <winsock2.h>
#include <stdlib.h>
#include <io.h>

#else

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sipstack/Data.hxx>

#endif

#include <sipstack/UdpTransport.hxx>
#include <sipstack/SipMessage.hxx>


using namespace std;
using namespace Vocal2;

const unsigned long
UdpTransport::MaxBufferSize = 64000;

UdpTransport::UdpTransport(int portNum, Fifo<SipMessage>& fifo) : 
   Transport(portNum, fifo)
{
   mFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if ( mFd < 0 )
   {
      //InfoLog (<< "Failed to open socket: " << portNum);
   }
   
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY); // !jf! 
   addr.sin_port = htons(portNum);
   
   if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) != 0 )
   {
      int err = errno;
      if ( err == EADDRINUSE )
      {
         //InfoLog (<< "Address already in use");
      }
      else
      {
         //InfoLog (<< "Could not bind to port: " << portNum);
      }
      
      throw TransportException("Address already in use", __FILE__,__LINE__);
   }

   // make non blocking 
#if WIN32
   unsigned long block = 0;
   int errNoBlock = ioctlsocket( mFd, FIONBIO , &block );
   assert( errNoBlock == 0 );
#else
   int flags  = fcntl( mFd, F_GETFL, 0);
   int errNoBlock = fcntl(mFd,F_SETFL, flags| O_NONBLOCK );
   assert( errNoBlock == 0 );
#endif

}

UdpTransport::~UdpTransport()
{
}

void 
UdpTransport::send( const sockaddr_in& dest, const  char* buffer, const size_t length) //, TransactionId txId)
{
   SendData* data = new SendData;
   data->destination = dest;
   data->buffer = buffer;
   data->length = length;
   //data->tid = txId;
   
   mTxFifo.add(data); // !jf!
}


void UdpTransport::process()
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   
   // how do we know that buffer won't get deleted on us !jf!
   if (mTxFifo.messageAvailable())
   {
      SendData* data = mTxFifo.getNext();
      unsigned int count = ::sendto(mFd, data->buffer, data->length, 0, (const sockaddr*)&data->destination, sizeof(data->destination));
   
      if ( count < 0 )
      {
         //DebugLog (<< strerror(errno));
         // !jf! what to do if it fails
         assert(0);
      }

      assert (count == data->length || count < 0);
   }
   
   
   // !jf! this may have to change - when we read a message that is too big
   char* buffer = new char[MaxBufferSize];
   struct sockaddr_in from;
   int fromLen = sizeof(from);
   
   // !jf! how do we tell if it discarded bytes 
   int len = recvfrom( mFd,
                       buffer,
                       MaxBufferSize,
                       0 /*flags */,
                       (struct sockaddr*)&from,
                       (socklen_t*)&fromLen);
   if ( len <= 0 )
   {
      int err = errno;
   }
   else if (len > 0)
   {
      cerr << "Received : " << len << " bytes" << endl;
      
      SipMessage* message = new SipMessage;
      
      message->addSource(from);
      
      // set the received from information into the received= parameter in the via
      // save the interface information in the message
      // preparse the message
      // stuff the message in the 
      
      mRxFifo.add(message);
   }
}

