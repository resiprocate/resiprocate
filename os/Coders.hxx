#if !defined(CODERS_HXX)
#define CODERS_HXX

#include "resiprocate/os/Data.hxx"

static const char* const resipCodersHeaderVersion = "$Id: Coders.hxx,v 1.8 2003/03/25 06:33:48 jason Exp $";

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
};


#endif
// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End
