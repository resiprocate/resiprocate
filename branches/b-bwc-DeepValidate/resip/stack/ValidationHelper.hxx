#ifndef ValidationHelper_Include_Guard
#define ValidationHelper_Include_Guard

#include "rutil/Data.hxx"

namespace resip
{
class ValidationHelper
{
   public:
      static bool checkIsUser(const Data& user);
      static bool checkIsUserInfo(const Data& userInfo);
      static bool checkIsUserAtHost(const Data& userAtHost);
      static bool checkIsTelSubscriber(const Data& telSub, bool allowParams);
      static bool checkIsHost(const Data& host);

   private:
      ValidationHelper();
};
}
#endif
