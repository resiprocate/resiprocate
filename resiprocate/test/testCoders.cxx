#include <stdlib.h>
#include <unistd.h>
#include <cassert>
#include <memory>

#include "sip2/util/compat.hxx"
#include "sip2/util/Coders.hxx"
#include <iostream>


using namespace std;
using namespace Vocal2;


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


void*
operator new(size_t size)
{
   static size_t poolSize = 16*1024*1024;
   static void *pool = 0;

   size += sizeof(void*) - size % sizeof(void*);
   if (size > poolSize || !pool)
   {
      poolSize=16*1024*1024;
      pool = malloc(poolSize);
      unsigned char a[4]; a[0] = 'P';
      write(2,a,sizeof(a));
      write(2,&pool,sizeof(void*));
      write(2,&poolSize,sizeof(size_t));
      
   }
   void * p = pool;
   pool = static_cast<void*>(static_cast<unsigned char*>(pool) + size);
   poolSize -= size;

   unsigned char a[4]; a[0] = 'N';
   write(2,a,sizeof(a));
   write(2,&p,sizeof(void*));
   write(2,&size,sizeof(size_t));
   return p;
   
}

void operator delete(void* p)
{
   void *z = 0;
   unsigned char a[4]; a[0] = 'D';
   write(2,a,sizeof(a));
   write(2,&p,sizeof(void*));
   write(2,&z,sizeof(void*));
   return;
}

void operator delete[](void* p)
{
   void *z = 0;
   unsigned char a[4]; a[0] = 'X';
   write(2,a,sizeof(a));
   write(2,&p,sizeof(void*));
   write(2,&z,sizeof(void*));
   return;
}


int
main()
{
   using namespace Vocal2;

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
