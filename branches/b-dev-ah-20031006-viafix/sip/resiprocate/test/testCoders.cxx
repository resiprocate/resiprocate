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
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
