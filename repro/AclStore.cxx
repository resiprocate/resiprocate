
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/sip/Uri.hxx"

#include "repro/AclStore.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


AclStore::AclStore(AbstractDb& db):
   mDb(db)
{  
}


AclStore::~AclStore()
{
}

      
void 
AclStore::addAcl(const resip::Data& acl )
{ 
   InfoLog( << "Add ACL" );
   
   AbstractDb::AclRecord rec;
   rec.mMachine = acl;
   
   mDb.addAcl( buildKey(acl), rec );
}

      
AclStore::DataList
AclStore::getAcls() const
{ 
   AbstractDb::AclRecordList input = mDb.getAllAcls();
   
   DataList result;
   result.reserve( input.size() );
   
   for (AbstractDb::AclRecordList::const_iterator it = input.begin();
        it != input.end(); it++)
   {
      result.push_back(it->mMachine);
   }
   return result;   
}


void 
AclStore::eraseAcl(const resip::Data& acl)
{  
   mDb.eraseAcl( buildKey(acl) );
}


AbstractDb::Key 
AclStore::buildKey(const resip::Data& acl ) const
{  
   return acl;
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
