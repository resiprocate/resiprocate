#if !defined(RESIP_CODERS_HXX)
#define RESIP_CODERS_HXX

#include "resiprocate/os/Data.hxx"

static const char* const resipCodersHeaderVersion =
   "$Id: Coders.hxx,v 1.9 2003/06/02 20:52:32 ryker Exp $";

namespace resip
{


class Base64Coder
{
        
   public:
      // encoded data is 4/3 rds length of input
      static Data encode(const Data&);
      
      // decoded data is 3/4s length of coded
      static Data decode(const Data&);

    private:
        static unsigned char toBits(unsigned char c);
        static unsigned char codeChar[];
        
};

}

#endif
