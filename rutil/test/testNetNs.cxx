#include <cassert>
#include <sys/types.h>
#include <ifaddrs.h>

#include "rutil/Logger.hxx"
#include "rutil/NetNs.hxx"
#include "rutil/HashMap.hxx"

using namespace resip;
using namespace std;

#ifdef USE_NETNS

typedef vector<Data> DataVector;

int getInterfaces(HashMap<Data, DataVector> & interfaces)
{
   int interfaceCount = 0;
   struct ifaddrs* interfaceList = NULL;

   getifaddrs(&interfaceList);
   struct ifaddrs* interface = interfaceList;
   while(interface)
   {
      if(interface->ifa_flags)
      {
         Data address = 
            inet_ntoa(((struct sockaddr_in*)interface->ifa_addr)->sin_addr);

         if(address.find(".0.0.0") == Data::npos)
         {
            interfaces[interface->ifa_name].push_back(address);
            //resipCerr << interface->ifa_name << "=" << address
            //    << "(" << interface->ifa_flags << ")" << std::endl;
            interfaceCount++;
         }
      }

      interface = interface->ifa_next;
   }

   return(interfaceCount);
}

bool testPublicNetNs()
{
   vector<Data> publicNetNs;
   int netnsCount = NetNs::getPublicNetNs(publicNetNs);
   //resipCerr << "netns count: " << netnsCount <<std::endl;
   //resipCerr << "vector size: " << publicNetNs.size() <<std::endl;

   assert(netnsCount);
   assert(netnsCount == (int) publicNetNs.size());
   for(unsigned int netNsIndex = 0; netNsIndex < publicNetNs.size(); netNsIndex++)
   {
      HashMap<Data, DataVector> currentInterfaces;

      resipCerr << "Found netns: \"" << publicNetNs[netNsIndex] << "\"" << std::endl;

      // Switch the current netns
      NetNs::setNs(publicNetNs[netNsIndex]);

      // Get the interfaces in the current netns
      getInterfaces(currentInterfaces);

      for(HashMap<Data, DataVector>::iterator interfaceItr = currentInterfaces.begin();
              interfaceItr != currentInterfaces.end(); ++ interfaceItr)
      {
         for(int addressIndex = 0; addressIndex < interfaceItr->second.size(); addressIndex++)
         {
            resipCerr << interfaceItr->first << "=" << interfaceItr->second[addressIndex] << std::endl;
         }
      }
   }

   // make sure we can change back to global netns
   resipCerr << "Switching back to default/global netns" << std::endl;
   NetNs::setNs("");
   HashMap<Data, DataVector> defaultInterfaces;
   getInterfaces(defaultInterfaces);

   // TODO: compare results with first time we switched to global/default netns
   for(HashMap<Data, DataVector>::iterator interfaceItr = defaultInterfaces.begin();
           interfaceItr != defaultInterfaces.end(); ++ interfaceItr)
   {
      for(int addressIndex = 0; addressIndex < interfaceItr->second.size(); addressIndex++)
      {
         resipCerr << interfaceItr->first << "=" << interfaceItr->second[addressIndex] << std::endl;
      }
   }



   return(false);
}

int main(int argc, const char* argv[])
{
    assert(!testPublicNetNs());

    resipCerr << "ALL OK" << std::endl;
    return(0);
}

#else

int main(int argc, const char* argv[])
{
    resipCerr << "USE_NETNS not set." << std::endl;
    return(0);
}

#endif

/* ====================================================================
 *
 * Copyright (c) 2014 Daniel Petrie, SIPez LLC  All rights reserved.
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
 * vi: set shiftwidth=3 expandtab:
 *
 * ====================================================================
 *
 */
