#if !defined(TESTTRANSPORT_HXX)
#define TESTTRANSPORT_HXX

#include <sipstack/Transport.hxx>
#include <sipstack/Message.hxx>
#include <sipstack/circular.hxx>

namespace Vocal2
{


  class TestReliableTransport : public Transport
  {
  public:
    TestReliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo);
    ~TestReliableTransport();
    
    void process(fd_set* fdSet=NULL);
    void buildFdSet( fd_set* fdSet, int* fdSetSize );
    
    bool isReliable() const;
    Type transport() const;

  };

  class TestUnreliableTransport : public Transport
  {
  public:
    TestUnreliableTransport(const Data& sendHost, int portNum, const Data& nic, Fifo<Message>& rxFifo);
    ~TestUnreliableTransport();
    
    void process(fd_set* fdSet=NULL);
    void buildFdSet( fd_set* fdSet, int* fdSetSize );
    
    bool isReliable() const;
    Type transport() const;

  };

  // singleton classes for input and output buffers

  typedef circular_buffer<char> TestBufType;


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

