
#include <assert.h>

#include <util/Coders.hxx>

namespace Vocal2
{

using namespace std;

unsigned char Base64Coder::codeChar[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";



string Base64Coder::encode(const unsigned char* data, int length)
{
    string rv;
    
    for(int index=0;index<length;index+=3)
    {
        unsigned char codeBits = (data[index] & 0xfc)>>2;
        
        assert(codeBits < 64);
        rv += codeChar[codeBits]; // c0 output
        


        // do second codeBits
        codeBits = ((data[index]&0x3)<<4);
        if (index+1 < length)
        {
            codeBits |= ((data[index+1]&0xf0)>>4);
        }
        assert(codeBits < 64);
        rv += codeChar[codeBits]; // c1 output
        
        if (index+1 >= length) break; // encoded d0 only
        

        // do third codeBits
        codeBits = ((data[index+1]&0xf)<<2);
        if (index+2 < length)
        {
            codeBits |= ((data[index+2]&0xc0)>>6);
        }
        assert(codeBits < 64);
        rv += codeChar[codeBits]; // c2 output
        
        if (index+2 >= length) break; // encoded d0 d1 only
        
        // do fourth codeBits
        codeBits = ((data[index+2]&0x3f));
        assert(codeBits < 64);
        rv += codeChar[codeBits]; // c3 output
        
        // outputed all d0,d1, and d2
    }
    return rv;
}

unsigned char Base64Coder::toBits(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c-'A';
    if (c >= 'a' && c <= 'z') return c-'a'+26;
    if (c >= '0' && c <= '9') return c-'0'+52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return 0;
}

int Base64Coder::decode(const string& source, unsigned char* data, int length)
{
    int c = 0;
    int srcLen = source.length();
    if (length < (srcLen*3)/4)
    {
        // diag in CVS
        return -1;
    }
    
    unsigned int out = 0;
    for( int index = 0 ; index < srcLen ; index+=4)
    {
        if (index+1 >= srcLen) break;
        
        unsigned char c0 = toBits(source[index]);
        unsigned char c1 = toBits(source[index+1]);
        
        data[out++] = 
            (c0 << 2) | 
            ((c1 & 0x30) >> 4) ;
        
        // done d0
        
        if (index + 2 >= srcLen) break; // no data for d1
        
        unsigned char c2 = toBits(source[index+2]);
        
        data[out++] = 
            ((c1 & 0xf) << 4 ) | 
            ((c2 & 0x3c) >> 2);
        
        // done d1
        
        if (index + 3 >= srcLen) break; // no data for d2
        
        unsigned char c3 = toBits(source[index+3]);
        
        data[out++] = ((c2&0x3) << 6) | c3;
        // done d2
    }
    return out;
}

 
};


// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End
