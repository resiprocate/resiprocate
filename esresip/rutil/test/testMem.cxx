/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */


#include <cassert>
#include <iostream>

#include "rutil/Timer.hxx"
#include "rutil/Random.hxx"


using namespace resip;
using namespace std;


int 
main(int argc, char* argv[])
{
   register unsigned int n=0;
   register unsigned int i=0;
   register unsigned int size=1;
   register int* data;
   unsigned int num = 1;

   if ( argc != 4 ) 
   {
      cerr << "Usage: testMem size num writeBool" << endl;
      cerr << " mem used is 2^size " << endl;
      cerr << "Example: ./testMem 22 1000000000 0" << endl;
      exit(1);
   }

   assert( sizeof(int) == 4);
   
   size = (1 << atoi( argv[1] )) - 1;
   num  = atoi( argv[2] );
   int write = atoi( argv[3] );
   clog << "Doing " << num << " interations on " << size*4 << " bytes " << endl;
   clog << "Doing " << num/1000000 << " M interations on " << size*4/1000000 << " M bytes " << endl;
   assert( num > 1 );
   assert( size > 1 );

   clog << "Filling mem ... ";
   // initalize the memory space
   Random::initialize();
   data = new int[size+1];
   for (i=0;i<=size;i++)
   {
      data[i] = (0x7FFF) & Random::getRandom();
      //clog << data[i] << endl;
   }
   clog << " Done" << endl;
   
   // compute the function across memory 
   UInt64 t1 = Timer::getTimeMicroSec();
   i=0;                                         
   n = num;
   if ( write == 0 )
   {
      while ( --n > 0 )
      {
         i = (data[i]+n) & size;
         //clog << i << endl;
      }
   }
   else
   {
      while ( --n > 0 )
      {
         data[i] |= 0x8000;
         i = (data[i]+n) & size;
         //clog << i << endl;
      }
   }
   UInt64 t2 = Timer::getTimeMicroSec();
   
   n=0;
   for (i=0;i<=size;i++)
   {
      if ( data[i] & 0x8000 ) n++;
   }
   clog << "Hit " << n << " out of " << size << " or " << n*100/size << "%" <<endl;
         
   clog << "Time is " << t2-t1 << " uSec" << endl;
   clog << "Time is " << (t2-t1)/1000000 << " seconds" << endl;
   assert ( t2-t1 > 0 );
   
   UInt64 rate = num;
   rate *= 1000000;
   rate = rate / (t2-t1);
   clog << "Rate is " << rate << " read/sec" << endl;
   rate = rate / 1000000;
   clog << "Rate is " << rate << " M read / sec" << endl;
   
   return 0;
}
/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
