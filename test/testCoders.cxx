#include <iostream>
#include <util/Coders.hxx>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string>

using namespace std;
using namespace Vocal2;


unsigned char * randomData(int size)
{
   unsigned char * p = new unsigned char[size];

   int fd = open("/dev/urandom",O_RDONLY);

   if (fd < 0 || (read(fd,p,size) != size))
   {
      delete p;
      p = 0;
   }
   return p;
}


void showData(const unsigned  char * data, int len)
{
   cout << "Data (n=" << len << "): ";
   for(int i = 0 ; i < len ; i++)
   {
      cout << hex << (unsigned int)data[i] << ' ';
   }
   cout << endl;

}

void showCoded(const unsigned  char * data, int len, const string& coded)
{
    showData(data,len);
    
//    cout << endl << " encoded: " << coded << endl;

}

int compareBuffers(unsigned char *b1, unsigned char *b2, int len)
{
   for(int i = 0 ; i < len ; i++)
   {
      if (b1[i] != b2[i]) return -(i+1);
   }
   return 0;
}

int
main()
{
  string testString("The quick brown fox jumped over the lazy dog.");

  int len = testString.length();

  string encoded =    
    Base64Coder::encode(
			 reinterpret_cast<const unsigned char*>
			 (testString.c_str()),len);

  unsigned char decoded[len+1];

  int retVal = Base64Coder::decode(encoded, decoded, len);
  if(retVal > 0) decoded[retVal] = 0;
  cout << "encoded: '" << encoded << "'" << endl;
  cout << "retVal: " << retVal << endl;
  cout << "decoded: '" << decoded << "'" << endl;

  int rVal = 0; // test return val
  for(int i=0;i<32;i++)
  {
     unsigned char * originalData = randomData(i);
     cout << "-------" << endl;
     
     if (!originalData)
     {
	cerr << " unable to allocate test case for " << i << " bytes " << endl;
	rVal = -1;
	break;
     }

     // encrypt this data

     string coded = Base64Coder::encode(originalData,i);

     showData(originalData,i);

     unsigned char recoveredData[i];

     int nbytes = Base64Coder::decode(coded, recoveredData, i);
     if (nbytes != i)
     {
	cout << i << ": unable to recover length " << nbytes << endl;
	rVal = -1;
     }

     showData(recoveredData, nbytes);

     cout << "encoded: " << coded << endl;
     
     int b = 0;
     if ((b = compareBuffers(originalData, recoveredData,i)) < 0)
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
