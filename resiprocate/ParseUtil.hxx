#if !defined(ParseUtil_hxx)
#define ParseUtil_hxx

namespace resip 
{

class Data;

class ParseUtil
{
   public:
      // full parse
      static bool isIpV6Address(const Data& addr);
      // fast and dirty
      static bool fastRejectIsIpV6Address(const Data& addr);
};

}
#endif
 
