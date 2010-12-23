#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

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

typedef boost::function<void (FakeUsage& f) > FakeUsageFunction;

int 
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   

//      DeferCall<FakeUsage> d(bind(&FakeUsage::doSomething, _1, 7, "bob"));
//      TestEndPoint.hxx:185:      typedef boost::function<bool (boost::shared_ptr<Event> e) > PredicateFn;

   FakeUsageFunction d;
//      boost::function<void, FakeUsage> d;
   d = bind(&FakeUsage::doSomething, _1, 7, "bob");
   InfoLog(<< "Call deferred");
   
   FakeUsage f;
   d(f);
}
                
         
      
               
         
