#if !defined(RESIP_TCPBASETRANSPORT_HXX)
#define RESIP_TCPBASETRANSPORT_HXX

#include "resiprocate/Transport.hxx"
#include "resiprocate/ConnectionManager.hxx"

namespace resip
{

class TransactionMessage;

class TcpBaseTransport : public Transport
{
   public:
      enum  {MaxFileDescriptors = 100000};

      TcpBaseTransport(Fifo<TransactionMessage>& fifo, int portNum,  IpVersion version, const Data& interfaceName );
      virtual  ~TcpBaseTransport();
      
      void process(FdSet& fdset);
      void buildFdSet( FdSet& fdset);
      bool isReliable() const { return true; }
      int maxFileDescriptors() const { return MaxFileDescriptors; }

      ConnectionManager& getConnectionManager() {return mConnectionManager;}

   protected:
      virtual Connection* createConnection(Tuple& who, Socket fd, bool server=false)=0;
      
      void processAllWriteRequests(FdSet& fdset);
      void sendFromRoundRobin(FdSet& fdset);
      void processListen(FdSet& fdSet);

      static const size_t MaxWriteSize;
      static const size_t MaxReadSize;

   private:
      static const int MaxBufferSize;
      ConnectionManager mConnectionManager;
};

}


#endif
