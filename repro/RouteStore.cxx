
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Lock.hxx"
#include "resip/stack/Uri.hxx"

#include "repro/RouteStore.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

bool RouteStore::RouteOp::operator<(const RouteOp& rhs) const
{
   return routeRecord.mOrder < rhs.routeRecord.mOrder;
}


RouteStore::RouteStore(AbstractDb& db):
   mDb(db)
{  
   Key key = mDb.firstRouteKey();
   while ( !key.empty() )
   {
      RouteOp route;
      route.routeRecord =  mDb.getRoute(key);
      route.key = key;
      route.preq = 0;
      
      if(!route.routeRecord.mMatchingPattern.empty())
      {
         int flags = REG_EXTENDED;
         if(route.routeRecord.mRewriteExpression.find("$") == Data::npos)
         {
            flags |= REG_NOSUB;
         }
         route.preq = new regex_t;
         int ret = regcomp(route.preq, route.routeRecord.mMatchingPattern.c_str(), flags);
         if(ret != 0)
         {
            delete route.preq;
            ErrLog(<< "Routing rule has invalid match expression: "
                   << route.routeRecord.mMatchingPattern);
            route.preq = 0;
         }
      }

      mRouteOperators.insert( route );

      key = mDb.nextRouteKey();
   } 
   mCursor = mRouteOperators.begin();
}


RouteStore::~RouteStore()
{
   for(RouteOpList::iterator i = mRouteOperators.begin(); i != mRouteOperators.end(); i++)
   {
      if ( i->preq )
      {
         regfree ( i->preq );
         delete i->preq;

         // !abr! Can't modify elements in a set
         // i->preq = 0;

      }
   }
   mRouteOperators.clear();
}

      
bool
RouteStore::addRoute(const resip::Data& method,
                     const resip::Data& event,
                     const resip::Data& matchingPattern,
                     const resip::Data& rewriteExpression,
                     const int order )
{ 
   InfoLog( << "Add route" );
   
   RouteOp route;

   Key key = buildKey(method, event, matchingPattern);
   
   if(findKey(key)) return false;

   route.routeRecord.mMethod = method;
   route.routeRecord.mEvent = event;
   route.routeRecord.mMatchingPattern = matchingPattern;
   route.routeRecord.mRewriteExpression =  rewriteExpression;
   route.routeRecord.mOrder = order;

   if(!mDb.addRoute(key , route.routeRecord))
   {
      return false;
   }

   route.key = key;
   route.preq = 0;
   if( !route.routeRecord.mMatchingPattern.empty() )
   {
     int flags = REG_EXTENDED;
     if( route.routeRecord.mRewriteExpression.find("$") == Data::npos )
     {
       flags |= REG_NOSUB;
     }
     route.preq = new regex_t;
     int ret = regcomp( route.preq, route.routeRecord.mMatchingPattern.c_str(), flags );
     if( ret != 0 )
     {
       delete route.preq;
       route.preq = 0;
     }
   }

   {
      WriteLock lock(mMutex);
      mRouteOperators.insert( route );
   }
   mCursor = mRouteOperators.begin(); 

   return true;
}

      
/*
AbstractDb::RouteRecordList 
RouteStore::getRoutes() const
{ 
   AbstractDb::RouteRecordList result;
   result.reserve(mRouteOperators.size());
   
   for (RouteOpList::const_iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      result.push_back(it->routeRecord);
   }
   return result;   
}
*/


void 
RouteStore::eraseRoute(const resip::Data& method,
                       const resip::Data& event,
                       const resip::Data& matchingPattern)
{
   Key key = buildKey(method, event, matchingPattern);
   eraseRoute(key);
}


void 
RouteStore::eraseRoute(const resip::Data& key )
{  
   mDb.eraseRoute(key);

   {
      WriteLock lock(mMutex);

      RouteOpList::iterator it = mRouteOperators.begin();
      while ( it != mRouteOperators.end() )
      {
         if (it->key == key )
         {
            RouteOpList::iterator i = it;
            it++;
            if ( i->preq )
            {
               regfree ( i->preq );
               delete i->preq;

               // !abr! Can't modify elements in a set
               //i->preq = 0;

            }
            mRouteOperators.erase(i);
         }
         else
         {
            it++;
         }
      }
   }
   mCursor = mRouteOperators.begin();  // reset the cursor since it may have been on deleted route
}


bool
RouteStore::updateRoute( const resip::Data& originalKey, 
                         const resip::Data& method,
                         const resip::Data& event,
                         const resip::Data& matchingPattern,
                         const resip::Data& rewriteExpression,
                         const int order )
{
   eraseRoute(originalKey);
   return addRoute(method, event, matchingPattern, rewriteExpression, order);
}


RouteStore::Key 
RouteStore::getFirstKey()
{
   ReadLock lock(mMutex);

   mCursor = mRouteOperators.begin();
   if ( mCursor == mRouteOperators.end() )
   {
      return Key( Data::Empty );
   }
   
   return mCursor->key;
}

bool 
RouteStore::findKey(const Key& key)
{ 
   // check if cursor happens to be at the key
   if ( mCursor != mRouteOperators.end() )
   {
      if ( mCursor->key == key )
      {
         return true;
      }
   }
   
   // search for the key 
   mCursor = mRouteOperators.begin();
   while (  mCursor != mRouteOperators.end() )
   {
      if ( mCursor->key == key )
      {
         return true; // found the key 
      }
      mCursor++;
   }
   return false; // key was not found 
}

RouteStore::Key 
RouteStore::getNextKey(Key& key)
{  
   ReadLock lock(mMutex);

   if ( !findKey(key) )
   {
      return Key(Data::Empty);
   }
      
   mCursor++;
   
   if ( mCursor == mRouteOperators.end() )
   {
      return Key( Data::Empty );
   }
   
   return mCursor->key;
}


AbstractDb::RouteRecord 
RouteStore::getRouteRecord(const resip::Data& key)
{
   ReadLock lock(mMutex);

   if (!findKey(key))
   {
      return AbstractDb::RouteRecord();
   }
   return mCursor->routeRecord;
}


RouteStore::UriList 
RouteStore::process(const resip::Uri& ruri, 
                    const resip::Data& method, 
                    const resip::Data& event )
{
   RouteStore::UriList targetSet;
   if(mRouteOperators.empty()) return targetSet;  // If there are no routes bail early to save a few cycles (size check is atomic enough, we don't need a lock)

   ReadLock lock(mMutex);

   for (RouteOpList::iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      DebugLog( << "Consider route " // << *it
                << " reqUri=" << ruri
                << " method=" << method 
                << " event=" << event );

      const AbstractDb::RouteRecord& rec = it->routeRecord;
      
      if(!rec.mMethod.empty())
      {
         if(!isEqualNoCase(rec.mMethod,method))
         {
            DebugLog( << "  Skipped - method did not match" );
            continue;
         }
         
      }
      if(!rec.mEvent.empty())
      {
         if(!isEqualNoCase(rec.mEvent, event))
         {
            DebugLog( << "  Skipped - event did not match" );
            continue;
         }
      }
      const Data& rewrite = rec.mRewriteExpression;
      const Data& match = rec.mMatchingPattern;
      if ( it->preq ) 
      {
         int ret;
         // TODO - !cj! www.pcre.org looks like it has better performance
         // !mbg! is this true now that the compiled regexp is used?
         Data uri;
         {
            DataStream s(uri);
            s << ruri;
            s.flush();
         }
         
         const int nmatch=10;
         regmatch_t pmatch[nmatch];
         
         ret = regexec(it->preq, uri.c_str(), nmatch, pmatch, 0/*eflags*/);
         if ( ret != 0 )
         {
            // did not match 
            DebugLog( << "  Skipped - request URI "<< uri << " did not match " << match );
            continue;
         }

         DebugLog( << "  Route matched" );
         Data target = rewrite;
         
         if ( rewrite.find("$") != Data::npos )
         {
            for ( int i=1; i<nmatch; i++)
            {
               if ( pmatch[i].rm_so != -1 )
               {
                  Data subExp(uri.substr(pmatch[i].rm_so,
                                         pmatch[i].rm_eo-pmatch[i].rm_so));
                  DebugLog( << "  subExpression[" <<i <<"]="<< subExp );

                  Data result;
                  {
                     DataStream s(result);

                     ParseBuffer pb(target);
                     
                     while (true)
                     {
                        const char* a = pb.position();
                        pb.skipToChars( Data("$") + char('0'+i) );
                        if ( pb.eof() )
                        {
                           s << pb.data(a);
                           break;
                        }
                        else
                        {
                           s << pb.data(a);
                           pb.skipN(2);
                           s <<  subExp;
                        }
                     }
                     s.flush();
                  }
                  target = result;
               }
            }
         }
         
         Uri targetUri;
         try
         {
            targetUri = Uri(target);
         }
         catch( BaseException& )
         {
            ErrLog( << "Routing rule transform " << rewrite << " gave invalid URI " << target );
            try
            {
               targetUri = Uri( Data("sip:")+target);
            }
            catch( BaseException& )
            {
               ErrLog( << "Routing rule transform " << rewrite << " gave invalid URI sip:" << target );
               continue;
            }
         }
         targetSet.push_back( targetUri );
      }
   }

   return targetSet;
}
  

RouteStore::Key 
RouteStore::buildKey(const resip::Data& method,
                     const resip::Data& event,
                     const resip::Data& matchingPattern ) const
{  
   // missing mOrder
   // Data pKey = Data(order) + ":" + method + ":" + event + ":" + matchingPattern;
   Data pKey = method+":"+event+":"+matchingPattern; 
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
