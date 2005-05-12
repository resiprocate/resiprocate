#include "resiprocate/TransactionUser.hxx"
#include "CurlHttpProvider.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/external/HttpProvider.hxx"
#include "resiprocate/external/HttpGetMessage.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

class TestCurlTransactionUser : public TransactionUser
{
   public:
      virtual const Data& name() const 
      { 
         static Data n("TestCurlTransactionUser");
         return n;
      }
      TimeLimitFifo<Message>& getFifo() { return mFifo; }
   private:
};

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   
   HttpProvider::setFactory(std::auto_ptr<HttpProviderFactory>(new CurlHttpProviderFactory()));
   
   GenericUri target;
   if (argc == 2)
   {
      InfoLog ( << "Looking up: " << argv[1]);
      target.uri() = argv[1];
   }
   else
   {
      target.uri() = "www.google.ca";
   }
   TestCurlTransactionUser testCurlTu;
   HttpProvider::instance()->get(target, "aaa", testCurlTu);
   
   Message* res = testCurlTu.getFifo().getNext(3000);
   if (res)
   {
      HttpGetMessage* msg = dynamic_cast<HttpGetMessage*>(res);
      assert(msg);
      InfoLog( << "Received: " << *msg);
      delete msg;
   }
   else
   {
      InfoLog( << "Reqeust timed out");
      exit(-1);
   }
   return 0;
}
