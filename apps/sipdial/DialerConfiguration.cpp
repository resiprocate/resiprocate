
#include <iostream>
#include <string>

#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"


using namespace resip;
using namespace std;

DialerConfiguration::DialerConfiguration()
{
}

DialerConfiguration::~DialerConfiguration()
{
}

void DialerConfiguration::loadStream(std::istream& in)
{
   while(!in.eof())
   {
      string param;
      string value;
      in >> param >> value;
      cerr << "param = " << param << " and value = " << value << endl;
      if(param == string("dialerIdentity"))
         setDialerIdentity(NameAddr(Uri(Data(value))));
      else if(param == string("authRealm"))
         setAuthRealm(Data(value));
      else if(param == string("authUser"))
         setAuthUser(Data(value));
      else if(param == string("authPassword"))
         setAuthPassword(Data(value));
      else if(param == string("callerUserAgentAddress"))
         setCallerUserAgentAddress(Uri(Data(value)));
      else if(param == string("callerUserAgentVariety"))
      {
         if(value == string("LinksysSPA941"))
            setCallerUserAgentVariety(LinksysSPA941);
         else if(value == string("PolycomIP501"))
            setCallerUserAgentVariety(PolycomIP501);
         else if(value == string("Cisco7940"))
            setCallerUserAgentVariety(Cisco7940);
         else if(value == string("Generic"))
            setCallerUserAgentVariety(Generic);
         else
            assert(0);   // FIXME
      }
      else if(param == string("targetPrefix"))
         setTargetPrefix(Data(value));
      else if(param == string("targetDomain"))
         setTargetDomain(Data(value)); 
/*      else if(param.size() > 0 && 
         (param[0] == '#' || param[0] == ';'))
         / / skip comment lines
         { } */
      else if(param == string(""))
         // skip empty lines
         { }
      else
         assert(0); // FIXME
   }
}


/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

