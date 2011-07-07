#if !defined EndPoint_hxx
#define EndPoint_hxx

#include "tfm/TestEndPoint.hxx"
#include "tfm/Expect.hxx"
#include "tfm/WrapperEvent.hxx"

class ExpectPredicate;

class EndPoint : public TestEndPoint
{
   public:
      virtual ~EndPoint();

      void handleEvent(Event* eventRaw);

      // Standard expects
      template<class T>
      ExpectBase* expect(int timeoutMs, ActionBase* expectAction)
      {
         return new Expect(*this,
                           new WrapperEventMatcher<T>,
                           timeoutMs,
                           expectAction);
      }

      template<class T>
      ExpectBase* expect(ExpectPredicate* pred, int timeoutMs, ActionBase* expectAction)
      {
         return new Expect(*this,
                           new WrapperEventMatcher<T>,
                           pred,
                           timeoutMs,
                           expectAction);
      }

    protected:
      virtual void clean();
};

#endif
