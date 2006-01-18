
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/Uri.hxx"

#include "repro/ConfigStore.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


ConfigStore::ConfigStore(AbstractDb& db):
   mDb(db)
{  
}


ConfigStore::~ConfigStore()
{
}

      
void 
ConfigStore::addDomain(const resip::Data& domain,
                       const int tlsPort )
{ 
   InfoLog( << "Add domain to config " );
   
   AbstractDb::ConfigRecord rec;
   rec.mDomain = domain;
   rec.mTlsPort = tlsPort;
   
   mDb.addConfig( buildKey(domain), rec );
}

      
/*AbstractDb::ConfigRecordList 
ConfigStore::getConfigs() const
{ 
   return mDb.getAllConfigs();
}
*/

ConfigStore::DataList 
ConfigStore::getDomains() const
{  
   AbstractDb::ConfigRecordList input = mDb.getAllConfigs();
   
   DataList result;
   result.reserve( input.size() );
   
   for (AbstractDb::ConfigRecordList::const_iterator it = input.begin();
        it != input.end(); it++)
   {
      result.push_back(it->mDomain);
   }
   return result;   
}


int      
ConfigStore::getTlsPort(const resip::Data& domain) const
{ 
   // this is a really lame way to implement - shoudl cache all the config data 

   AbstractDb::ConfigRecordList input = mDb.getAllConfigs();
   
   DataList result;
   result.reserve( input.size() );
   
   for (AbstractDb::ConfigRecordList::const_iterator it = input.begin();
        it != input.end(); it++)
   {
      if ( it->mDomain == domain )
      {
         return it->mTlsPort;
      }
   }
   
   return 0; 
}


void 
ConfigStore::eraseDomain(const resip::Data& domain)
{  
   mDb.eraseConfig( buildKey(domain) );
}


AbstractDb::Key 
ConfigStore::buildKey(const resip::Data& domain ) const
{  
   return domain;
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
 */
