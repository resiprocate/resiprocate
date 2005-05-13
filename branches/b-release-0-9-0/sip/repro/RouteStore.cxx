
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/Uri.hxx"

#include "repro/RouteStore.hxx"


using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


RouteStore::RouteStore(AbstractDb& db):
   mDb(db)
{  
   Key key = mDb.firstRouteKey();
   while ( !key.empty() )
   {
      RouteOp route;
      route.routeRecord =  mDb.getRoute(key);

      // TODO - !cj! - compile regex when create the route object instead of
      // doing it every time 
      
      mRouteOperators.push_back( route ); 

      key = mDb.nextRouteKey();
   }
}


RouteStore::~RouteStore()
{
}

      
void 
RouteStore::add(const resip::Data& method,
                const resip::Data& event,
                const resip::Data& matchingPattern,
                const resip::Data& rewriteExpression,
                const int order )
{ 
   InfoLog( << "Add route" );
   
   RouteOp route;
   route.routeRecord.mMethod = method;
   route.routeRecord.mEvent = event;
   route.routeRecord.mMatchingPattern = matchingPattern;
   route.routeRecord.mRewriteExpression =  rewriteExpression;
   route.routeRecord.mOrder = order;
 
   // TODO - !cj! - compile regex when create the route object instead of
   // doing it every time 
   
   mRouteOperators.push_back( route ); 

   mDb.addRoute( buildKey(method,event,matchingPattern), route.routeRecord );
}

      
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


void 
RouteStore::erase(const resip::Data& method,
                  const resip::Data& event,
                  const resip::Data& matchingPattern )
{  
   // !cj! TODO 
   assert(0);
}


RouteStore::UriList 
RouteStore::process(const resip::Uri& ruri, 
                    const resip::Data& method, 
                    const resip::Data& event )
{
   RouteStore::UriList targetSet;

   for (RouteOpList::iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      DebugLog( << "Consider route " // << *it
                << " reqUri=" << ruri
                << " method=" << method 
                << " event=" << event );

      AbstractDb::RouteRecord& rec = it->routeRecord;
      
      if ( !rec.mMethod.empty() )
      {
         if ( rec.mMethod != method)
         {
            DebugLog( << "  Skipped - method did not match" );
            continue;
         }
         
      }
      if ( !rec.mEvent.empty() )
      {
         if ( rec.mEvent != event) 
         {
            DebugLog( << "  Skipped - event did not match" );
            continue;
         }
      }
      Data& rewrite = rec.mRewriteExpression;
      Data& match = rec.mMatchingPattern;

      if ( !match.empty() ) 
      {
         // TODO - www.pcre.org looks like it has better performance 
                  
         // TODO - !cj! - compile regex when create the route object instead of
         // doing it every time 
         int flags = REG_EXTENDED;
         if ( rewrite.find("$") == Data::npos )
         {
            flags |= REG_NOSUB;
         }
         int ret = regcomp(&(it->preq),match.c_str(), flags );
         if ( ret != 0 )
         { 
            ErrLog( << "Routing rule has invalid match expression: " 
                    << match );
            continue;
         }
         
         Data uri;
         {
            DataStream s(uri);
            s << ruri;
            s.flush();
         }
         
         const int nmatch=10;
         regmatch_t pmatch[nmatch];
         
         ret = regexec(&(it->preq), uri.c_str(), nmatch, pmatch, 0/*eflags*/);
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
         catch( BaseException& e)
         {
            ErrLog( << "Routing rule transform " << rewrite << " gave invalid URI " << target );
            try
            {
               targetUri = Uri( Data("sip:")+target);
            }
            catch( BaseException& e)
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
