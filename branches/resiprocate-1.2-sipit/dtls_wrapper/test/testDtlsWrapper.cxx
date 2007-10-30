#include <iostream>

#include "DtlsFactory.hxx"
#include "DtlsSocket.hxx"

using namespace std;
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
        cout << "Hey, amazing, it worked\n";
      }
      
      virtual void handshakeFailed()
      {
        cout << "Bummer, handshake failure\n";
        exit(-1);
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

   cout << "Created the factory\n";
}
     
