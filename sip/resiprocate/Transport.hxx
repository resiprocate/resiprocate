#if !defined(TRANSPORT_HXX)
#define TRANSPORT_HXX

#include <util/Data.hxx>
#include <exception>

#ifndef WIN32
#include <netinet/in.h>
#endif

#include <util/Fifo.hxx>
#include <util/Socket.hxx>
#include <sipstack/Message.hxx>

namespace Vocal2
{

class SipMessage;

class SendData
      {
	  public:
		  SendData();

            sockaddr_in& destination;
            //TransactionId tid;
            const char* buffer;
            size_t length;
      };

class Transport
{
   public:
      

      class TransportException : public std::exception
      {
         public:
            TransportException(const Data& msg, const Data& file, const int line);
            virtual const char* what() const throw();
      };

      Transport(int portNum, Fifo<Message>& rxFifo);
      // !ah! need to think about type for
      // interface specification here.
      
      virtual ~Transport();
      
      virtual void send( const sockaddr_in& address, const  char* buffer, size_t length)=0; //, TransactionId txId) = 0;

      virtual void process() = 0 ;

      void run();               // will not return.
      
      void shutdown();

   protected:
	Socket mFd; // this is a unix file descriptor or a windows SOCKET
      int mPort;
      Fifo<SendData> mTxFifo; // owned by the transport
      Fifo<Message>& mStateMachineFifo; // passed in

   private:

      bool mShutdown ;
};

}
#endif
