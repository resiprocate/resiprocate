
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Lock.hxx"

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ExtensionHeader.hxx"

#include "repro/FilterStore.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

bool FilterStore::FilterOp::operator<(const FilterOp& rhs) const
{
   return filterRecord.mOrder < rhs.filterRecord.mOrder;
}


FilterStore::FilterStore(AbstractDb& db):
   mDb(db)
{  
   Key key = mDb.firstFilterKey();
   while ( !key.empty() )
   {
      FilterOp filter;
      filter.filterRecord =  mDb.getFilter(key);
      filter.key = key;
      filter.pcond1 = 0;
      filter.pcond2 = 0;
      
      int flags = REG_EXTENDED;
      if(filter.filterRecord.mActionData.find("$") == Data::npos)
      {
         flags |= REG_NOSUB;
      }

      if(!filter.filterRecord.mCondition1Regex.empty())
      {
         filter.pcond1 = new regex_t;
         int ret = regcomp(filter.pcond1, filter.filterRecord.mCondition1Regex.c_str(), flags);
         if(ret != 0)
         {
            delete filter.pcond1;
            ErrLog( << "Condition1Regex has invalid match expression: "
                   << filter.filterRecord.mCondition1Regex);
            filter.pcond1 = 0;
         }
      }

      if(!filter.filterRecord.mCondition2Regex.empty())
      {
         filter.pcond2 = new regex_t;
         int ret = regcomp(filter.pcond2, filter.filterRecord.mCondition2Regex.c_str(), flags);
         if(ret != 0)
         {
            delete filter.pcond2;
            ErrLog( << "Condition2Regex has invalid match expression: "
                   << filter.filterRecord.mCondition2Regex);
            filter.pcond2 = 0;
         }
      }

      mFilterOperators.insert(filter);

      key = mDb.nextFilterKey();
   } 
   mCursor = mFilterOperators.begin();
}


FilterStore::~FilterStore()
{
   for(FilterOpList::iterator i = mFilterOperators.begin(); i != mFilterOperators.end(); i++)
   {
      if (i->pcond1)
      {
         regfree(i->pcond1);
         delete i->pcond1;
      }
      if (i->pcond2)
      {
         regfree(i->pcond2);
         delete i->pcond2;
      }
   }
   mFilterOperators.clear();
}


bool 
FilterStore::addFilter(const resip::Data& cond1Header,
                       const resip::Data& cond1Regex,
                       const resip::Data& cond2Header,
                       const resip::Data& cond2Regex,
                       const resip::Data& method,
                       const resip::Data& event,
                       short action,
                       const resip::Data& actionData,
                       const short order)
{ 
   InfoLog( << "Add filter" );
   
   FilterOp filter;

   Key key = buildKey(cond1Header, cond1Regex, cond2Header, cond2Regex, method, event);
   
   if(findKey(key)) return false;

   filter.filterRecord.mCondition1Header = cond1Header;
   filter.filterRecord.mCondition1Regex = cond1Regex;
   filter.filterRecord.mCondition2Header = cond2Header;
   filter.filterRecord.mCondition2Regex = cond2Regex;
   filter.filterRecord.mMethod = method;
   filter.filterRecord.mEvent = event;
   filter.filterRecord.mAction = action;
   filter.filterRecord.mActionData =  actionData;
   filter.filterRecord.mOrder = order;

   if(!mDb.addFilter(key , filter.filterRecord))
   {
      return false;
   }

   filter.key = key;
   filter.pcond1 = 0;
   filter.pcond2 = 0;
   int flags = REG_EXTENDED;
   if(filter.filterRecord.mActionData.find("$") == Data::npos)
   {
      flags |= REG_NOSUB;
   }
   if(!filter.filterRecord.mCondition1Regex.empty())
   {
      filter.pcond1 = new regex_t;
      int ret = regcomp(filter.pcond1, filter.filterRecord.mCondition1Regex.c_str(), flags);
      if(ret != 0)
      {
         delete filter.pcond1;
         filter.pcond1 = 0;
      }
   }
   if(!filter.filterRecord.mCondition2Regex.empty())
   {
      filter.pcond2 = new regex_t;
      int ret = regcomp(filter.pcond2, filter.filterRecord.mCondition2Regex.c_str(), flags);
      if(ret != 0)
      {
         delete filter.pcond2;
         filter.pcond2 = 0;
      }
   }

   {
      WriteLock lock(mMutex);
      mFilterOperators.insert( filter );
   }
   mCursor = mFilterOperators.begin(); 

   return true;
}

      
/*
AbstractDb::FilterRecordList 
FilterStore::getFilters() const
{ 
   AbstractDb::FilterRecordList result;
   result.reserve(mFilterOperators.size());
   
   for (FilterOpList::const_iterator it = mFilterOperators.begin();
        it != mFilterOperators.end(); it++)
   {
      result.push_back(it->filterRecord);
   }
   return result;   
}
*/


void 
FilterStore::eraseFilter(const resip::Data& cond1Header,
                         const resip::Data& cond1Regex,
                         const resip::Data& cond2Header,
                         const resip::Data& cond2Regex,
                         const resip::Data& method,
                         const resip::Data& event)
{
   Key key = buildKey(cond1Header, cond1Regex, cond2Header, cond2Regex, method, event);
   eraseFilter(key);
}


void 
FilterStore::eraseFilter(const resip::Data& key)
{  
   mDb.eraseFilter(key);

   {
      WriteLock lock(mMutex);

      FilterOpList::iterator it = mFilterOperators.begin();
      while (it != mFilterOperators.end())
      {
         if (it->key == key)
         {
            FilterOpList::iterator i = it;
            it++;
            if(i->pcond1)
            {
               regfree(i->pcond1);
               delete i->pcond1;
            }
            if(i->pcond2)
            {
               regfree(i->pcond2);
               delete i->pcond2;
            }
            mFilterOperators.erase(i);
         }
         else
         {
            it++;
         }
      }
   }
   mCursor = mFilterOperators.begin();  // reset the cursor since it may have been on deleted filter
}


bool
FilterStore::updateFilter(const resip::Data& originalKey,
                          const resip::Data& cond1Header,
                          const resip::Data& cond1Regex,
                          const resip::Data& cond2Header,
                          const resip::Data& cond2Regex,
                          const resip::Data& method,
                          const resip::Data& event,
                          short action,
                          const resip::Data& actionData,
                          const short order)
{
   eraseFilter(originalKey);
   return addFilter(cond1Header, cond1Regex, cond2Header, cond2Regex, method, event, action, actionData, order);
}


FilterStore::Key 
FilterStore::getFirstKey()
{
   ReadLock lock(mMutex);

   mCursor = mFilterOperators.begin();
   if ( mCursor == mFilterOperators.end() )
   {
      return Key( Data::Empty );
   }
   
   return mCursor->key;
}

bool 
FilterStore::findKey(const Key& key)
{ 
   // check if cursor happens to be at the key
   if ( mCursor != mFilterOperators.end() )
   {
      if ( mCursor->key == key )
      {
         return true;
      }
   }
   
   // search for the key 
   mCursor = mFilterOperators.begin();
   while (  mCursor != mFilterOperators.end() )
   {
      if ( mCursor->key == key )
      {
         return true; // found the key 
      }
      mCursor++;
   }
   return false; // key was not found 
}

FilterStore::Key 
FilterStore::getNextKey(Key& key)
{  
   ReadLock lock(mMutex);

   if ( !findKey(key) )
   {
      return Key(Data::Empty);
   }
      
   mCursor++;
   
   if ( mCursor == mFilterOperators.end() )
   {
      return Key( Data::Empty );
   }
   
   return mCursor->key;
}


AbstractDb::FilterRecord 
FilterStore::getFilterRecord(const resip::Data& key)
{
   ReadLock lock(mMutex);

   if (!findKey(key))
   {
      return AbstractDb::FilterRecord();
   }
   return mCursor->filterRecord;
}


void
FilterStore::getHeaderFromSipMessage(const SipMessage& msg, const Data& headerName, list<Data>& headerList)
{
   // First see if header string is "request-line"
   if(isEqualNoCase(headerName, "request-line"))
   {
      headerList.push_back(Data::from(msg.header(h_RequestLine)));
      return;
   }
  
   // Next check to see if it is a standard header
   Headers::Type headerType = Headers::getType(headerName.c_str(), headerName.size());
   if(headerType != Headers::UNKNOWN)
   {
      Data headerData;
      const HeaderFieldValueList* hfv = msg.getRawHeader(headerType);
      for(HeaderFieldValueList::const_iterator it = hfv->begin(); it != hfv->end(); it++)
      {
         it->toShareData(headerData);
         headerList.push_back(headerData);
      }
   }
   else // Check if custom header
   {
      ExtensionHeader exHeader(headerName);
      if(msg.exists(exHeader))
      {
         const StringCategories& exHeaders = msg.header(exHeader);
         for(StringCategories::const_iterator it = exHeaders.begin(); it != exHeaders.end(); it++)
         {
            headerList.push_back(it->value());
         }
      }
   }
}

bool 
FilterStore::applyRegex(int conditionNum, const Data& header, const Data& match, regex_t *regex, Data& rewrite)
{
   int ret;
   resip_assert(conditionNum < 10);
   
   // TODO - !cj! www.pcre.org looks like it has better performance
   // !mbg! is this true now that the compiled regexp is used?

   const int nmatch=10;  // replacements $x1-$x9 are allowed, where x is the condition number
   regmatch_t pmatch[nmatch];

   ret = regexec(regex, header.c_str(), nmatch, pmatch, 0/*eflags*/);
   if (ret != 0)
   {
      // did not match 
      return false;
   }

   DebugLog( << "  Filter matched: header=" << header << ", regex=" << match);

   if (rewrite.find("$") != Data::npos)
   {
      for (int i=1; i<nmatch; i++)
      {
         if (pmatch[i].rm_so != -1)
         {
            Data subExp(header.substr(pmatch[i].rm_so,
                                      pmatch[i].rm_eo-pmatch[i].rm_so));
            DebugLog( << "  subExpression[" <<i <<"]="<< subExp );

            Data result;
            {
               DataStream s(result);
               ParseBuffer pb(rewrite);

               while (true)
               {
                  const char* a = pb.position();
                  pb.skipToChars(Data("$") + char('0' + conditionNum) + char('0' + i));
                  if (pb.eof())
                  {
                     s << pb.data(a);
                     break;
                  }
                  else
                  {
                     s << pb.data(a);
                     pb.skipN(3);
                     s <<  subExp;
                  }
               }
               s.flush();
            }
            rewrite = result;
         }
      }
   }
   return true;
}

bool
FilterStore::process(const SipMessage& request, 
                     short& action,
                     Data& actionData)
{
   if(mFilterOperators.empty()) return false;  // If there are no filters bail early to save a few cycles (size check is atomic enough, we don't need a lock)

   ReadLock lock(mMutex);

   Data method(request.methodStr());
   Data event(request.exists(h_Event) ? request.header(h_Event).value() : Data::Empty);

   for (FilterOpList::iterator it = mFilterOperators.begin();
        it != mFilterOperators.end(); it++)
   {
      const AbstractDb::FilterRecord& rec = it->filterRecord;

      if(!rec.mMethod.empty())
      {
         if(!isEqualNoCase(rec.mMethod, method))
         {
            DebugLog( << "  Skipped - method did not match" );
            continue;
         }
      }
      if(!rec.mEvent.empty())
      {
         if(!isEqualNoCase(rec.mEvent,event)) 
         {
            DebugLog( << "  Skipped - event did not match" );
            continue;
         }
      }

      // Get requests SIP headers from SipMessage
      list<Data> condition1Headers;
      list<Data> condition2Headers;
      actionData = rec.mActionData;
      if(!rec.mCondition1Header.empty() && it->pcond1)
      {
         getHeaderFromSipMessage(request, rec.mCondition1Header, condition1Headers);

         // Check condition 1 regex
         list<Data>::iterator hit = condition1Headers.begin();
         bool match = false;
         for(; hit != condition1Headers.end() && match == false; hit++)
         {
            match = applyRegex(1, *hit, rec.mCondition1Regex, it->pcond1, actionData);
            DebugLog( << "  Cond1 HeaderName=" << rec.mCondition1Header << ", Value=" << *hit << ", Regex=" << rec.mCondition1Regex << ", match=" << match);
         }
         if(!match)
         {
            DebugLog( << "  Skipped - request did not match first condition: " << request.brief());
            continue;
         }
      }
      if(!rec.mCondition2Header.empty() && it->pcond2)
      {
         getHeaderFromSipMessage(request, rec.mCondition2Header, condition2Headers);

         // Check condition 2 regex
         list<Data>::iterator hit = condition2Headers.begin();
         bool match = false;
         for(; hit != condition2Headers.end() && match == false; hit++)
         {
            match = applyRegex(2, *hit, rec.mCondition2Regex, it->pcond2, actionData);
            DebugLog( << "  Cond2 HeaderName=" << rec.mCondition2Header << ", Value=" << *hit << ", Regex=" << rec.mCondition2Regex << ", match=" << match);
         }
         if(!match)
         {
            DebugLog( << "  Skipped - request did not match second condition: " << request.brief());
            continue;
         }
      }
      // If we make it here Method, Event and both conditions matched - return configured action
      action = rec.mAction;
      return true;
   }

   // If we make it here, then none of the conditions matched - return false
   return false;
}


bool 
FilterStore::test(const resip::Data& cond1Header, 
                  const resip::Data& cond2Header,
                  short& action,
                  resip::Data& actionData)
{
   ReadLock lock(mMutex);

   for (FilterOpList::iterator it = mFilterOperators.begin();
        it != mFilterOperators.end(); it++)
   {
      const AbstractDb::FilterRecord& rec = it->filterRecord;
      actionData = rec.mActionData;

      // Check condition 1 regex
      if(!rec.mCondition1Header.empty() && it->pcond1)
      {
         if(!applyRegex(1, cond1Header, rec.mCondition1Regex, it->pcond1, actionData))
         {
            continue;
         }
      }

      // Check condition 2 regex
      if(!rec.mCondition2Header.empty() && it->pcond2)
      {
         if(!applyRegex(2, cond2Header, rec.mCondition2Regex, it->pcond2, actionData))
         {
            continue;
         }
      }

      // If we make it here both conditions matched - return configured action
      action = rec.mAction;
      return true;
   }

   // If we make it here, then none of the conditions matched - return false
   return false;
}


FilterStore::Key 
FilterStore::buildKey(const resip::Data& cond1Header,
                      const resip::Data& cond1Regex,
                      const resip::Data& cond2Header,
                      const resip::Data& cond2Regex,
                      const resip::Data& method,
                      const resip::Data& event) const
{  
   Data pKey = cond1Header + ":" + cond1Regex + ":" + 
               cond2Header + ":" + cond2Regex + ":" + 
               method + ":" + event; 
   return pKey;
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
