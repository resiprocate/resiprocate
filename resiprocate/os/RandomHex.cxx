#include <stdio.h>
#include <openssl/rand.h>
#include <util/RandomHex.hxx>
#include <util/Lock.hxx>

using namespace Vocal2;

void
RandomHex::initialize()
{
   assert(RAND_status() == 1);
}

Data
RandomHex::get(unsigned int len)
{
   unsigned char buffer[len];
   int ret = RAND_bytes(buffer, len);
   assert (ret == 1);
   
   Data result;
   result = convertToHex(buffer, len);
   
   return result;
}

Data 
RandomHex::convertToHex(const unsigned char* src, int len)
{
    Data data;

    unsigned char temp;

    //convert to hex.
    int i;
    //int j = 0;

    for (i = 0; i < len; i++)
    {
        temp = src[i];

        int hi = (temp & 0xf0) / 16;
        int low = (temp & 0xf);

        char buf[4];
        buf[0] = '\0';

        sprintf(buf, "%x%x", hi, low);
        data += buf;
    }

    return data;
}

