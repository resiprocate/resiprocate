#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace dtls;


class TestDtlsSocketContext : public DtlsSocketContext
{
   public:
     //memory is only valid for duration of callback; must be copied if queueing
     //is required 
      virtual ~TestDtlsSocketContext(){}      
      virtual void write(const char* data, unsigned int len)
      {
      }
      
      virtual void handshakeCompleted()
      {
      }
      
      virtual void handshakeFailed()
      {
         assert(0);
      }
};


class TestTimerContext: public DtlsTimerContext{
     void addTimer(DtlsTimer *timer, unsigned int){
       delete timer;
     }
};

     
int main(int argc,char **argv)
{
   DtlsFactory *factory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()));
}
     
