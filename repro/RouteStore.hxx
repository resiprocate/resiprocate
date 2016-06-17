#if !defined(REPRO_ROUTESTORE_HXX)
#define REPRO_ROUTESTORE_HXX

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include <set>

#include "rutil/Data.hxx"
#include "rutil/RWMutex.hxx"
#include "resip/stack/Uri.hxx"

#include "repro/AbstractDb.hxx"


namespace repro
{
//class AbstractDb;

class RouteStore
{
   public:
      typedef std::vector<resip::Uri> UriList;
      typedef resip::Data Key;
      
      RouteStore(AbstractDb& db);
      ~RouteStore();
      
      bool addRoute(const resip::Data& method,
                    const resip::Data& event,
                    const resip::Data& matchingPattern,
                    const resip::Data& rewriteExpression,
                    const short order );
      
      void eraseRoute(const resip::Data& method,
                      const resip::Data& event,
                      const resip::Data& matchingPattern,
                      const short order);
      void eraseRoute( const resip::Data& key );

      bool updateRoute( const resip::Data& originalKey,
                        const resip::Data& method,
                        const resip::Data& event,
                        const resip::Data& matchingPattern,
                        const resip::Data& rewriteExpression,
                        const short order );
      
      AbstractDb::RouteRecord getRouteRecord(const resip::Data& key);
      
      Key getFirstKey();// return empty if no more
      Key getNextKey(Key& key); // return empty if no more 
      
      UriList process(const resip::Uri& ruri, 
                      const resip::Data& method, 
                      const resip::Data& event );

   private:
      bool findKey(const Key& key); // move cursor to key
      
      Key buildKey(const resip::Data& method,
                   const resip::Data& event,
                   const resip::Data& matchingPattern,
                   const short order) const;

      AbstractDb& mDb;  

      class RouteOp
      {
         public:
            Key key;
            regex_t *preq;
            AbstractDb::RouteRecord routeRecord;
            bool operator<(const RouteOp&) const;
      };
      
      resip::RWMutex mMutex;
      typedef std::multiset<RouteOp> RouteOpList;
      RouteOpList mRouteOperators; 
      RouteOpList::iterator mCursor;
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
