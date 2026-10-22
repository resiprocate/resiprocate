
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"
#include "DialInstance.hxx"

using namespace resip;
using namespace std;

Data getFullFilename()
{
#ifdef _MSC_VER
   char homeDrive[256] = { 0 };
   char homePath[256] = { 0 };
   size_t requiredSize = 0;
   getenv_s(&requiredSize, homeDrive, sizeof(homeDrive), "HOMEDRIVE");
   resip_assert(requiredSize > 0); // FIXME
   getenv_s(&requiredSize, homePath, sizeof(homePath), "HOMEPATH");
   resip_assert(requiredSize > 0); // FIXME
   return Data(string(homeDrive) + string(homePath) + string("/sipdial/sipdial.cfg"));
#else   
   char* home_dir = getenv("HOME");
   resip_assert(home_dir); // FIXME
   return Data(string(home_dir) + string("/.sipdial/sipdial.cfg"));
#endif
}

int main(int argc, char *argv[]) 
{
   Data defaultConfig(getFullFilename());
   DialerConfiguration dc;
   dc.parseConfig(argc, argv, defaultConfig, 1);

   Data targetUri(argv[1]);
   
   DialInstance di(dc, Uri(targetUri));
   di.execute();

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

