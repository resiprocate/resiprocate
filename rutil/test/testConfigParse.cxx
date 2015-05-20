#include "rutil/ConfigParse.hxx"
#include <cassert>
#include <iostream>

using namespace resip;
using namespace std;

class MyConfigParse : public ConfigParse
{
public:
   MyConfigParse() {};
   virtual void printHelpText(int argc, char **argv)
   {
      cout << "Testing ConfigParse::printHelpText" << std::endl;
   }
};

int
main(int argc, char *argv[])
{
   Data defaultConfigFilename("testConfigParse-1.config");
   MyConfigParse cp;
   cp.parseConfig(argc, argv, defaultConfigFilename);

   {
      // Does not exist in the sample config file
      Data param0 = cp.getConfigData("Param0", "");
      assert(param0.empty());
   }

   {
      // Does not exist in the sample config file
      // Test use of a default value
      Data param0 = cp.getConfigData("Param0", "unit testing");
      assert(param0 == "unit testing");
   }

   {
      Data param1 = cp.getConfigData("Param1", "");
      assert(param1 == "unit testing is good");
   }

   {
      ConfigParse::NestedConfigMap m = cp.getConfigNested("Foo");
      assert(m.size() == 0);
   }

   {
      ConfigParse::NestedConfigMap m = cp.getConfigNested("Transport");
      assert(m.size() == 2);

      ConfigParse& n1 = m[1];
      ConfigParse& n101 = m[101];

      Data protocol1 = n1.getConfigData("Protocol", "UDP");
      assert(protocol1 == "TLS");
      int port1 = n1.getConfigInt("Port", 5060);
      assert(port1 == 5061);

      Data protocol101 = n101.getConfigData("Protocol", "UDP");
      assert(protocol101 == "WSS");
      int port101 = n101.getConfigInt("Port", 8443);
      assert(port101 == 443);
   }

   return 0;
}

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock.  All rights reserved.
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

