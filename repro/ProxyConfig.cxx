#include "ProxyConfig.hxx"

#include <iostream>
#include <fstream>
#include <iterator>

//#include <rutil/DnsUtil.hxx>
#include <resip/stack/NameAddr.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

namespace repro 
{

ProxyConfig::ProxyConfig() : mStore(0)
{
}

ProxyConfig::~ProxyConfig()
{
   delete mStore; 
   mStore=0;
}

bool 
ProxyConfig::getConfigValue(const resip::Data& name, resip::Uri &value)
{
   Data lowerName(name);  lowerName.lowercase();
   ConfigValuesMap::iterator it = mConfigValues.find(lowerName);
   if(it != mConfigValues.end())
   {
      try
      {
         if(!it->second.empty())
         {
            NameAddr tempNameAddr(it->second);
            value = tempNameAddr.uri();
            return true;
         }
         else
         {
            value = Uri();  // return an empty Uri
            return true;
         }
      }
      catch(resip::BaseException& e)
      {
         // Try adding sip: to value to see if it will be valid
         try
         {
            NameAddr tempNameAddr(Data("sip:" + it->second));
            value = tempNameAddr.uri();
            return true;
         }
         catch(resip::BaseException&)
         {
            cerr << "Invalid Uri setting:  " << name << " = " << it->second << ": " << e << endl;
            return false;
         }
      }  
   }
   // Not found
   return false;
}

resip::Uri 
ProxyConfig::getConfigUri(const resip::Data& name, const resip::Uri defaultValue, bool useDefaultIfEmpty)
{
   Uri ret(defaultValue);
   if(getConfigValue(name, ret) && ret.host().empty() && useDefaultIfEmpty)
   {
      return defaultValue;
   }
   return ret;
}

void
ProxyConfig::printHelpText(int argc, char **argv)
{
   cout << "Command line format is:" << endl;
   cout << "  " << removePath(argv[0]) << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue>] [--<ConfigValueName>=<ConfigValue>] ..." << endl;
   cout << "Sample Command lines:" << endl;
   cout << "  " << removePath(argv[0]) << " repro.config --RecordRouteUri=sip:proxy.sipdomain.com --ForceRecordRouting=true" << endl;
   cout << "  " << removePath(argv[0]) << " repro.config /RecordRouteUri:sip:proxy.sipdomain.com /ForceRecordRouting:true" << endl;
}

void 
ProxyConfig::createDataStore(AbstractDb* db, AbstractDb* runtimedb)
{
   resip_assert(db);
   mStore = new Store(*db, runtimedb);
}

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
