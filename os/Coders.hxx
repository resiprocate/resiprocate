#if !defined(CODERS_HXX)
#define CODERS_HXX

#include "sip2/util/Data.hxx"

namespace Vocal2
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
