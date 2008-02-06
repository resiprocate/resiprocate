#ifndef DTLS_Timer_hxx
#define DTLS_Timer_hxx

namespace dtls
{

class DtlsTimer
{
   public:
      DtlsTimer(unsigned int seq); 
      virtual ~DtlsTimer();
      
      virtual void expired()=0;
      virtual void fire();
      unsigned int getSeq() { return mSeq; }
      //invalid could call though to context and call an external cancel
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
      //could add cancel here
   protected:
      void fire(DtlsTimer *timer);
};
   

}

#endif
