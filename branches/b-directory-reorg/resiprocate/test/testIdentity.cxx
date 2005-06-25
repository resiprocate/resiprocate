
#include <cassert>
#include <fstream>
#include <ostream>

#include "rutil/Logger.hxx"
#include "resiprocate/Security.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Debug, Data::Empty);

#ifdef USE_SSL
   Security* security=0;
   try
   {
      security = new Security;
   }
   catch( ... )
   {
      security = 0;
      ErrLog( << "Got a exception setting up Security" );
   }

   try
   {
      assert(security != 0);
      security->preload();
   }
   catch( ... )
   {
      ErrLog( << "Got a exception loading certificates" );
   }

   assert( security );

   Data in("sip:alice@atlanta.example.com"
           ":a84b4c76e66710"
           ":314159 INVITE"
           ":Thu, 21 Feb 2002 13:02:03 GMT"
           ":sip:alice@pc33.atlanta.example.com"
           ":v=0\r\n"
           "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
           "s=Session SDP\r\n"
           "c=IN IP4 pc33.atlanta.example.com\r\n"
           "t=0 0\r\n"
           "m=audio 49172 RTP/AVP 0\r\n"
           "a=rtpmap:0 PCMU/8000\r\n");
      
   ofstream strm("identiy-in", std::ios_base::trunc);
   strm.write( in.data(), in.size() );
   strm.flush();
   
   Data res = security->computeIdentity( Data("localhost"), in );

   //ErrLog( << "input is encoded " << in.charEncoded()  );
   //ErrLog( << "input is hex " << in.hex() );
   //ErrLog( << "input is  " << in );
   //ErrLog( << "identity is " << res  );

   bool c  = security->checkIdentity( Data("localhost"), in , res );
   
   if ( !c )
   {
      ErrLog( << "Identity check failed" << res  );
   }
   
   
#endif // use_ssl 

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
