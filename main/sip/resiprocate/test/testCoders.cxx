#include <iostream>
#include <unistd.h>
#include <string>
#include <assert.h>

#include "sip2/util/Coders.hxx"

using namespace std;
using namespace Vocal2;


Data randomData(int size)
{
   Data rv(size);

   for(int i = 0 ; i < size-1 ; i++)
   {
      rv += (char)i&0xff;
   }
   rv += '\0';
   return rv;
}


void showData(const Data& data)
{
   cout << "Data (n=" << data.size() << "): ";
   for(Data::size_type i = 0 ; i < data.size() ; i++)
   {
      cout << hex << (unsigned int)data.data()[i] << ' ';
   }
   cout << endl;
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
  Data testData("The quick brown fox jumped over the lazy dog.");

  Data encoded =    
     Base64Coder::encode(testData);

  Data decoded = Base64Coder::decode(encoded);

  cout << "encoded: '" << encoded << "'" << endl;
  cout << "decoded: '" << decoded << "'" << endl;


  int rVal = 0; // test return val
  for(int i=0;i<320;i++)
  {
     Data originalData = randomData(i);
     cout << i << "-------" << endl;


     // encrypt this data

     Data coded = Base64Coder::encode(originalData);

//     showData(originalData);

     Data decoded = Base64Coder::decode(coded);

     assert(originalData.size() == decoded.size());

//     showData(decoded);

     cout << "encoded: " << coded << endl;
     
     int b = 0;
     if ( originalData != decoded )
     {
	cout << i << ": symetry failure (encode/decode) at byte " << -b-1 << endl;
	rVal = -1;
     }
  }
  return rVal;

}
// Local Variables:
// mode:c++
// c-file-style:"ellemtel"
// c-file-offsets:((case-label . +))
// indent-tabs-mode:nil
// End
