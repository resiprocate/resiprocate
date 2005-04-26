
#include <fcntl.h>

#ifdef WIN32
#include <db.h>
#else 
#include <db4/db.h>
#endif

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include <cassert>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/Uri.hxx"

#include "repro/RouteDbMemory.hxx"

using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


RouteDbMemory::RouteDbMemory(char* dbName)
{  
   InfoLog( << "Loading route database" );
  
   mDb = new Db( NULL , 0 );
   assert( mDb );
   
   // if the line bellow seems wrong, you need to check which version 
   // of db you have - it is likely an very out of date version 
   // still trying to figure this out so email fluffy if you have 
   // problems and include your version the DB_VERSION_STRING found 
   // in your db4/db.h file. 
   int ret = mDb->open(NULL,dbName,NULL,DB_BTREE,DB_CREATE,0);
   //int ret = mDb->open(   dbName,NULL,DB_BTREE,DB_CREATE,0);

   if ( ret != 0 )
   {
      ErrLog( <<"Could not open route database at " << dbName );
	  assert(0);
   }
   assert(mDb);

   Dbt key;
   Dbt data;   
   Dbc* cursor;
   
    ret = mDb->cursor(NULL,&cursor,0);
    assert( cursor );
	assert( ret == 0 );

   assert( mDb );
   ret = cursor->get(&key,&data, DB_FIRST);

   while( ret == 0 ) // while key is being found 
   {
      Data d(reinterpret_cast<const char*>(data.get_data()), data.get_size() );
      DebugLog( << "loaded route " << d);

	  if ( d.empty() )
	  {
		// this should never happen 
		  ErrLog( <<"got an empty route record" );
		 // !cj! TODO assert(0);
		break;
	  }

      Route r = deSerialize( d );
      add( r.mMethod, r.mEvent, r.mMatchingPattern, r.mRewriteExpression, r.mOrder );
      
      ret = cursor->get(&key,&data, DB_NEXT);
   }
   cursor->close();
}


RouteDbMemory::~RouteDbMemory()
{ 
   int ret = mDb->close(0); 
   assert( ret == 0 );
   mDb = 0;
}


void 
RouteDbMemory::add(const resip::Data& method,
                   const resip::Data& event,
                   const resip::Data& matchingPattern,
                   const resip::Data& rewriteExpression,
                   const int order )
{
   InfoLog( << "Add route" );
   
   RouteOp route;
   route.mVersion = 1;
   route.mMethod = method;
   route.mEvent = event;
   route.mMatchingPattern = matchingPattern;
   route.mRewriteExpression =  rewriteExpression;
   route.mOrder = order;
   
   mRouteOperators.push_back( route );  
   
   Data pKey = method+" "+event+" "+matchingPattern; 
   Data pData = serialize(route);
      
   Dbt key( (void*)pKey.data(), (u_int32_t)pKey.size() );
   Dbt data( (void*)pData.data(), (u_int32_t)pData.size() );
 
   int ret;
 
   assert( mDb );
   ret = mDb->put(NULL,&key,&data,0);
   assert( ret == 0 );

   // need sync for if program gets ^C without shutdown
   ret = mDb->sync(0);
   assert( ret == 0 );
}


RouteDbMemory::RouteList 
RouteDbMemory::getRoutes() const
{
   RouteDbMemory::RouteList result;
   result.reserve(mRouteOperators.size());
   
   for (RouteOpList::const_iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      result.push_back(*it);
   }
   return result;   
}


void 
RouteDbMemory::erase(const resip::Data& method,
                       const resip::Data& event,
                       const resip::Data& matchingPattern )
{
   // TODO 
   assert(0);
}

      
RouteAbstractDb::UriList
RouteDbMemory::process(const resip::Uri& ruri, 
                       const resip::Data& method, 
                       const resip::Data& event )
{
   RouteAbstractDb::UriList targetSet;

#if 0
   Regex r;
#endif

   for (RouteOpList::iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      DebugLog( << "Consider route " // << *it
                << " reqUri=" << ruri
                << " method=" << method 
                << " event=" << event );
      
      if ( !it->mMethod.empty() )
      {
         if ( it->mMethod != method)
         {
            DebugLog( << "  Skipped - method did not match" );
            continue;
         }
         
      }
      if ( !it->mEvent.empty() )
      {
         if ( it->mEvent != event) 
         {
            DebugLog( << "  Skipped - event did not match" );
            continue;
         }
      }
      Data& rewrite = it->mRewriteExpression;
      Data& match = it->mMatchingPattern;

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
