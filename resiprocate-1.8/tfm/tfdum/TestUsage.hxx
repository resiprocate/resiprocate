#if !defined(TestUsage_hxx)
#define TestUsage_hxx

#include "tfm/EndPoint.hxx"

class DumUserAgent;

class TestUsage : public EndPoint
{
   public:
      TestUsage(DumUserAgent* ua);
      virtual ~TestUsage();

      DumUserAgent* getDumUserAgent() const;
      virtual bool isMyEvent(Event*)=0;

   protected:
      DumUserAgent* mUa;
};

#endif 
