#if !defined(REPRO_STORE_HXX)
#define REPRO_STORE_HXX

#include "repro/AbstractDb.hxx"
#include "repro/UserStore.hxx"
#include "repro/RouteStore.hxx"
#include "repro/AclStore.hxx"
#include "repro/ConfigStore.hxx"
#include "repro/StaticRegStore.hxx"
#include "repro/FilterStore.hxx"
#include "repro/SiloStore.hxx"

namespace repro
{
class  AbstractDb;

/** The Store contains differnt types of stores such as user, route, etc. These
 * use the AbstractDb to persist their data. */
class Store
{
   public:
      // If a seperate instance of AbstractDb is desired for the runtime database tables
      // (ie. UserStore and SiloStore), then is can be provided, otherwise pass NULL.
      // The Users and MessageSilo database tables are different from the other repro 
      // configuration database tables, in that they are accessed at runtime as SIP 
      // requests arrive.  It may be desirable to use BerkeleyDb for the other repro 
      // tables (which are read at starup time, then cached in memory), and MySQL for 
      // the runtime accessed tables; or two seperate MySQL instances for these different
      // table sets.
      Store(AbstractDb& db, AbstractDb* runtimedb=0);
      ~Store();
      
      UserStore mUserStore;
      RouteStore mRouteStore; 
      AclStore mAclStore; 
      ConfigStore mConfigStore;
      StaticRegStore mStaticRegStore;
      FilterStore mFilterStore;
      SiloStore mSiloStore;
    private:
};

}
#endif  

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
