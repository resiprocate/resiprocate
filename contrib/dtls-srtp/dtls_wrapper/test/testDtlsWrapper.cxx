#include "../DtlsFactory.hxx"

class TestTimerContext: public DtlsTimerContext{
     void addTimer(DtlsTimer *timer, unsigned int){
       delete timer;
     }
}
     
int main(int argc,char **argv)
  {
    DtlsFactory *factory=new DtlsFactory(std::auto_ptr<DtlsTimerContext>(new TestTimerContext()));
  }
     
