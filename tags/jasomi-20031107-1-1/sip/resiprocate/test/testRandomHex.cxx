
#include <iostream>
#include <set>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Random.hxx"

using namespace std;
using namespace resip;

int main(int argc, char** argv)
{
   if(argc != 3)
   {
      cerr << "usage: testRandomHex number_of_tries string_length" << endl;
      exit(-1);
   }

   int runs = atoi(argv[1]);
   int length = atoi(argv[2]);

   Random::initialize();
   

   if (runs <= 0 || length <= 0)
   {
      cerr << "usage: testRandomHex number_of_tries string_length" << endl
           << "number_of_tries and string_length must be a positive integers." << endl;
      exit(-1);
   }

   cerr << "Generating " << runs << " random " << length << " byte strings and checking for uniqueness." << endl;

   set<Data> randomDatas;
   
   for (int i = 0; i < runs; i++)
   {
      Data foo = Random::getRandomHex(length/2);
      cerr << foo << endl;
      if (randomDatas.insert(foo).second == false)
      {
         cerr << "RandomHex produced a duplicate" << length << "byte string after " << i << " runs. " << endl;
         exit(-1);
      }
   }

   cerr << endl << "Now doing crypto random" << endl;
   
   for (int i = 0; i < runs; i++)
   {
      Data foo = Random::getCryptoRandomHex(length/2);
      cerr << foo << endl;
      if (randomDatas.insert(foo).second == false)
      {
         cerr << "RandomHex produced a duplicate" << length << "byte string after " << i << " runs. " << endl;
         exit(-1);
      }
   }

   cerr << "Success." << endl;
   return 0;
}


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
