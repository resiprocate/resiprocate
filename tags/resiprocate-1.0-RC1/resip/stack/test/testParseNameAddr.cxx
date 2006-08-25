#include <iostream>

#include "TestSupport.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/ParserCategories.hxx"
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
class TR
{
   private:
      ostream& os;
      Data label;

      TR(const TR&);

      void show(const char * s)
      {
	 os << s << ' ' << label << endl;
      }

      void start()
      {
	 show(">>-");
      }

      void end()
      {
	 show("<<-");
      }

   public:
      TR(Data  s,ostream& o = cerr ):os(o),label(s) { start(); }
      TR(const char* l,ostream& o = cerr):os(o),label(l) { start(); }
      ~TR() { end();}
};

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;
   Log::initialize(Log::Cout, l, argv[0]);

   {
       TR _tr("Test odd name-addr from odd vendor");
       Data data(
           "~U90vw09jlMMs0OY7IIHzdD72c3<sip:~U90vw09jlMMs0OY7IIHzdD72c3@7.8.4.9:5066>");
       Data alt ("alternate");

       NameAddr illegal(data);
       cerr<< "++ host: " << illegal.uri().host() << endl;
       cerr<< "++ display-name: " << illegal.displayName() << endl;
       cerr<< "++ user-info: " << illegal.uri().user() << endl;
       assert( illegal.uri().host() == "7.8.4.9");
       assert( illegal.displayName() == "~U90vw09jlMMs0OY7IIHzdD72c3");
       assert( illegal.uri().port() == 5066 );
       cerr << "++ " << illegal << endl;
       illegal.displayName() = alt;
       cerr << "++ " << illegal << endl;
       assert( illegal.displayName() == alt);

   }
   return 0;
   cerr << "All OK" << endl;
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
