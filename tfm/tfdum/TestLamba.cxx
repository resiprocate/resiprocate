#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include <functional>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;


class FakeUsage
{
   public:
      void doSomething(int i, Data d)
      {
         InfoLog(<< "doSomething called:" << i << " " << d);
      }
};

typedef std::function<void(FakeUsage& f)> FakeUsageFunction;

int 
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   

//      DeferCall<FakeUsage> d(bind(&FakeUsage::doSomething, _1, 7, "bob"));
//      TestEndPoint.hxx:185:      typedef std::function<bool(std::shared_ptr<Event> e)> PredicateFn;

   FakeUsageFunction d;
//      std::function<void, FakeUsage> d;
   d = std::bind(&FakeUsage::doSomething, std::placeholders::_1, 7, "bob");
   InfoLog(<< "Call deferred");
   
   FakeUsage f;
   d(f);
}
                
         
      
               
         
