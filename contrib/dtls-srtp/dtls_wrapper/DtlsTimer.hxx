#ifndef DTLS_Timer_hxx
#define DTLS_Timer_hxx

namespace dtls
{

class DtlsTimer
{
   public:
      Timer(unsigned int seq); 
      ~Timer() {}
      
      virtual void expired() = 0;
      unsigned int getSeq() { return mSeq; }
   private:
      unsigned int mSeq;
      bool mValid;            
};


class DtlsTimerContext
{
   public:
      void addTimer(DtlsTimer* timer)=0;      
      
};
   

}

#endif
