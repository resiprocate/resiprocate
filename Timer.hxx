#if !defined(TIMER_HXX)
#define TIMER_HXX
     
#include <sipstack/Data.hxx>

#if defined( WIN32 )
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long UInt64;
#endif


namespace Vocal2
{

class Timer
{
   public:
      typedef unsigned long Id;
      typedef enum 
      {
         TimerA,
         TimerB,
         TimerC,
         TimerD,
         TimerE1,
         TimerE2,
         TimerF,
         TimerG,
         TimerH,
         TimerI,
         TimerJ,
         TimerK
      } Type;
      
      Timer(unsigned long ms, Type type, const Data& transactionId);
      Timer(unsigned long ms); // for TimerQueue only - don't use
      Timer(const Timer& t);
      Timer& operator=(const Timer& t);
      
      // returns the unique identifier
      Id getId() const { return mId; }
      
      static void setupTimeOffsets(); // initialize
      static UInt64 getTimeMicroSec(); // get a 64 bit time
      static UInt64 getTimeMs(); // in ms
      
   private:
      static UInt64 getSystemTime();
      static UInt64 getSystemTicks();

      UInt64 mWhen;
      Id mId;
      Type mType;
      Data mTransactionId;
      
      static unsigned long mCpuSpeedMHz;
      static UInt64 mBootTime;
      static unsigned long mTimerCount;

      friend bool operator<(const Timer& t1, const Timer& t2);
      friend class TimerQueue;
};
 

}

#endif
