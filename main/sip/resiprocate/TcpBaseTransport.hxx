#if !defined(RESIP_TCPBASETRANSPORT_HXX)
#define RESIP_TCPBASETRANSPORT_HXX

#include "resiprocate/Transport.hxx"
#include "resiprocate/ConnectionManager.hxx"

namespace resip
{

class SipMessage;

class TcpBaseTransport : public Transport
{
   public:
      TcpBaseTransport(Fifo<Message>& fifo, int portNum, const Data& sendhost, bool ipv4);
      virtual  ~TcpBaseTransport();

      void process(FdSet& fdset);
      void buildFdSet( FdSet& fdset);
      bool isReliable() const { return true; }

      ConnectionManager& getConnectionManager() {return mConnectionManager;}

   protected:
      virtual Connection* createConnection(Tuple& who, Socket fd, bool server=false)=0;
      
      void processSomeWrites(FdSet& fdset);
      void processSomeReads(FdSet& fdset);
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
