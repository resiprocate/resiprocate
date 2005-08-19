#include <assert.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/HeaderHash.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/MethodHash.hxx"
#include "resip/stack/ParameterTypes.hxx"
#include "resip/stack/ParameterHash.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static bool
testHeaderHash(bool verbose)
{
  bool gotErrors = false;

  if (verbose)
    cerr << "Test header hashing" << endl;

  // HeaderNames start from 1, in effect; not sure why.
  //
  int nHeaderNames = sizeof(Headers::HeaderNames) / sizeof(Data);
  if (nHeaderNames != (Headers::MAX_HEADERS + 1))
  {
    if (verbose)
      cerr << "Mismatch: " << nHeaderNames << " header names vs. "
	   << Headers::MAX_HEADERS << " header types" << endl;
    gotErrors = true;
  } 
  else
    for (int ht = 0; ht < Headers::MAX_HEADERS; ht++)
    {
      const Data& hName = Headers::getHeaderName(ht);
      Headers::Type ht1 = Headers::getType(hName.c_str(), hName.size());
      if (ht != ht1)
      {
	if (verbose)
	  cerr << "Header " << ht << " => " << hName << " => " << ht1 << endl;
	gotErrors = true;
      }
    }

  return gotErrors;
}


static bool
testMethodHash(bool verbose)
{
  bool gotErrors = false;

  if (verbose)
    cerr << "Test method hashing" << endl;


    for (int mt = 0; mt < MAX_METHODS; mt++)
    {
        const Data& mName = getMethodName(static_cast<MethodTypes>(mt));
//    cout << "Checking " << mName << endl;
      MethodTypes mt1 = getMethodType(mName);
      if (mt != mt1)
      {
	if (verbose)
	  cerr << "Method " << mt << " => " << mName << " => " << mt1 << endl;
	gotErrors = true;;
      }
    }

  return gotErrors;
}

static bool
testParameterHash(bool verbose)
{
  bool gotErrors = false;
  if (verbose)
    cerr << "Test parameter hashing" << endl;

  int nParameterNames = sizeof(ParameterTypes::ParameterNames) / sizeof(Data);
  if (nParameterNames != ParameterTypes::MAX_PARAMETER)
  {
    if (verbose)
      cerr << "Mismatch: " << nParameterNames << " parameter names vs. "
	   << ParameterTypes::MAX_PARAMETER << " parameter types" << endl;
    gotErrors = true;
  } 
  else
    for (int pt = 0; pt < ParameterTypes::MAX_PARAMETER; pt++)
    {
      const Data& pName = ParameterTypes::ParameterNames[pt];
//    cout << "Checking " << pName << endl;
      ParameterTypes::Type pt1 = ParameterTypes::getType(pName.c_str(),
							 pName.size());
      if (pt != pt1 && pt1 != ParameterTypes::qopFactory) // qop is weird
      {
	if (verbose)
	  cerr << "Parameter " << pt << " => " << pName << " => " << pt1 << endl;
	gotErrors = true;
      }
    }

  return gotErrors;
}

int main()
{
  unsigned errors = 0;

  if (testHeaderHash(true))
    errors |= 1;

  if (testMethodHash(true))
    errors |= 2;

  if (testParameterHash(true))
    errors |= 4;

  if (!errors)
    cerr << "All OK" << endl;

  return errors;
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
