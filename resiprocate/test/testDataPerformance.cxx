#include "sip2/util/DataStream.hxx"
#include "sip2/util/Random.hxx"

using namespace Vocal2;

int 
main()
{
   Data data = Random::getRandomHex(16);
   for (int j=0; j<100; j++)
   {
      Data output(1000000,true);
      DataStream strm(output);
      
      for (int i=0; i<100000; i++)
      {
         strm << data;
      }
   }
}
