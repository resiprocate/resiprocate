#if !defined(REPRO_FILTERSTORE_HXX)
#define REPRO_FILTERSTORE_HXX

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include <set>
#include <list>

#include "rutil/Data.hxx"
#include "rutil/RWMutex.hxx"

#include "repro/AbstractDb.hxx"

namespace resip
{
   class SipMessage;
}

namespace repro
{

class FilterStore
{
   public:
      typedef resip::Data Key;
      
      enum FilterResult
      {
         Accept,
         Reject,
         SQLQuery
      };

      FilterStore(AbstractDb& db);
      ~FilterStore();
      
      bool addFilter(const resip::Data& cond1Header,
                     const resip::Data& cond1Regex,
                     const resip::Data& cond2Header,
                     const resip::Data& cond2Regex,
                     const resip::Data& method,
                     const resip::Data& event,
                     short action,
                     const resip::Data& actionData,
                     const short order);
      
      void eraseFilter(const resip::Data& cond1Header,
                       const resip::Data& cond1Regex,
                       const resip::Data& cond2Header,
                       const resip::Data& cond2Regex,
                       const resip::Data& method,
                       const resip::Data& event);
      void eraseFilter(const resip::Data& key);

      bool updateFilter(const resip::Data& originalKey,
                        const resip::Data& cond1Header,
                        const resip::Data& cond1Regex,
                        const resip::Data& cond2Header,
                        const resip::Data& cond2Regex,
                        const resip::Data& method,
                        const resip::Data& event,
                        short action,
                        const resip::Data& actionData,
                        const short order);
      
      AbstractDb::FilterRecord getFilterRecord(const resip::Data& key);
      
      Key getFirstKey();// return empty if no more
      Key getNextKey(Key& key); // return empty if no more 

      bool process(const resip::SipMessage& request, 
                   short& action,
                   resip::Data& actionData);

      bool test(const resip::Data& cond1Header, 
                const resip::Data& cond2Header,
                short& action,
                resip::Data& actionData);

   private:
      bool findKey(const Key& key); // move cursor to key
      
      Key buildKey(const resip::Data& cond1Header,
                   const resip::Data& cond1Regex,
                   const resip::Data& cond2Header,
                   const resip::Data& cond2Regex,
                   const resip::Data& method,
                   const resip::Data& event) const;

      void getHeaderFromSipMessage(const resip::SipMessage& msg, 
                                   const resip::Data& headerName, 
                                   std::list<resip::Data>& headerList);
      bool applyRegex(int conditionNum,
                      const resip::Data& header, 
                      const resip::Data& match, 
                      regex_t *regex, 
                      resip::Data& rewrite);

      AbstractDb& mDb;  

      class FilterOp
      {
         public:
            Key key;
            regex_t *pcond1;
            regex_t *pcond2;
            AbstractDb::FilterRecord filterRecord;
            bool operator<(const FilterOp&) const;
      };
      
      resip::RWMutex mMutex;
      typedef std::multiset<FilterOp> FilterOpList;
      FilterOpList mFilterOperators; 
      FilterOpList::iterator mCursor;
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
