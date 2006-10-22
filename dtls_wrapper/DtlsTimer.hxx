#ifndef DTLS_Timer_hxx
#define DTLS_Timer_hxx

namespace dtls
{

class DtlsTimer
{
   public:
      DtlsTimer(unsigned int seq); 
      virtual ~DtlsTimer();
      
      virtual void expired() = 0;
      unsigned int getSeq() { return mSeq; }
      void invalidate() { mValid = false; }
   private:
      unsigned int mSeq;
      bool mValid;            
};


class DtlsTimerContext
{
   public:
      virtual ~DtlsTimerContext() {}
      virtual void addTimer(DtlsTimer* timer, unsigned int waitMs)=0;      
};
   

}

#endif
