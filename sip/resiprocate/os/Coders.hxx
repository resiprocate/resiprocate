#if !defined(_J_CODERS_HXX)
#define _J_CODERS_HXX

#include <string>


// DO NOT INCLUDE THIS FILE DIRECTLY. INCLUDE jutil.hxx INSTEAD.

namespace Vocal2
{


class Base64Coder
{
        
    public:
        /// Return a C++ sting representing the data of length length.
        static std::string encode(const unsigned char* data, int length);
        
        /// decode the sourceString and place in data, write no more than
        //  length bytes. Return nbytes or < 0 for error.
        static int decode(const std::string& sourceString,unsigned char *data, int length);

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
