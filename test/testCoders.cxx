#include <stdlib.h>
#include <unistd.h>
#include <cassert>
#include <memory>

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Coders.hxx"
#include <iostream>

#ifdef __FreeBSD__
#define NEW_THROWS 1
#else
#define NEW_THROWS 0
#endif

using namespace std;
using namespace resip;


Data* randomData(int size)
{
   unsigned char * p = new unsigned char[size];

   for(int i = 0 ; i < size; i++)
   {
      p[i] = static_cast<unsigned char>(random()&0xff);
   }

   return new Data(p,size);
}


void showData(const Data& data)
{
   cout << "Data (n=" << data.size() << "): ";
   for(Data::size_type i = 0 ; i < data.size() ; i++)
   {
      cout << hex << (unsigned int)(((unsigned char *)(data.data()))[i]) << ' ';
   }
   cout << dec << endl;
}

void showCoded(const Data& data)
{
   showData(data);
}

int compareData(const Data &a, const Data& b)
{
   return a == b;
}

int
main()
{
   using namespace resip;

   assert(sizeof(size_t) == sizeof(void*));

  Data testData("The quick brown fox jumped over the lazy dog.");

  Data encoded =    
     Base64Coder::encode(testData);

  Data decoded = Base64Coder::decode(encoded);

  cout << "encoded: '" << encoded << "'" << endl;
  cout << "decoded: '" << decoded << "'" << endl;


  int rVal = 0; // test return val
  for(int i=1;i<320;i++)
  {
     Data* originalData = randomData(i);

     cout << i << "-------" << endl;


     // encrypt this data

     Data coded = Base64Coder::encode(*originalData);

     showData(*originalData);

     Data decoded = Base64Coder::decode(coded);

     assert(originalData->size() == decoded.size());

     showData(decoded);

     cout << "encoded: " << coded << endl;
     
     int b = 0;
     if ( *originalData != decoded )
     {
	cout << i << ": symetry failure (encode/decode) at byte " << -b-1 << endl;
	rVal = -1;
     }
     delete originalData;
  }
  return rVal;

}
// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End
