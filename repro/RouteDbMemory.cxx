
#include <fcntl.h>
#include <db4/db_185.h>
#include <cassert>

#include <regex.h>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Uri.hxx"

#include "repro/RouteDbMemory.hxx"

using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


RouteDbMemory::RouteDbMemory(char* dbName)
{  
   InfoLog( << "Loading route database" );
   
   mDb = dbopen(dbName,O_CREAT|O_RDWR,0000600,DB_BTREE,0);
   if ( !mDb )
   {
      ErrLog( <<"Could not open user database at " << dbName );
   }
   assert(mDb);

   DBT key,data;
   int ret;
   
   assert( mDb );
   ret = mDb->seq(mDb,&key,&data, R_FIRST);
   assert( ret != -1 );
   assert( ret != 2 );

   while( ret == 0 ) // while key is being found 
   {
      Data d(reinterpret_cast<const char*>(data.data), data.size );
      DebugLog( << "loaded route " << d);

      Route r = deSerialize( d );
      add( r.mMethod, r.mEvent, r.mMethod, r.mRewriteExpression, r.mOrder );
      
      // get the next key 
      ret = mDb->seq(mDb,&key,&data, R_NEXT);
      assert( ret != -1 );
      assert( ret != 2 );
   }
}


RouteDbMemory::~RouteDbMemory()
{ 
   int ret = mDb->close(mDb);
   assert( ret == 0 );
}


void 
RouteDbMemory::add(const resip::Data& method,
                   const resip::Data& event,
                   const resip::Data& matchingPattern,
                   const resip::Data& rewriteExpression,
                   const int order )
{
   InfoLog( << "Add route" );
   
   RouteOperator route;
   route.mVersion = 1;
   route.mMethod = method;
   route.mEvent = event;
   route.mMatchingPattern = matchingPattern;
   route.mRewriteExpression =  rewriteExpression;
   route.mOrder = order;
   
   mRouteOperators.push_back( route );  
   
   Data pKey = method+" "+event+" "+matchingPattern; 
   Data pData = serialize(route);
      
   DBT key,data;
   int ret;
   key.data = const_cast<char*>( pKey.data() );
   key.size = pKey.size();
   data.data = const_cast<char*>( pData.data() );
   data.size = pData.size();
   assert( mDb );
   ret = mDb->put(mDb,&key,&data,0);
   assert( ret == 0 );

   // need sync for if program gets ^C without shutdown
   ret = mDb->sync(mDb,0);
   assert( ret == 0 );
}


RouteDbMemory::RouteList 
RouteDbMemory::getRoutes() const
{
   RouteDbMemory::RouteList result;
   result.reserve(mRouteOperators.size());
   
   for (RouteOperatorList::const_iterator it = mRouteOperators.begin();
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
   RouteAbstractDb::UriList ret;
   
   for (RouteOperatorList::iterator it = mRouteOperators.begin();
        it != mRouteOperators.end(); it++)
   {
      if (it->matches(ruri, method, event))
      {
         ret.push_back( it->transform(ruri) );
      }
   }
   return ret;
}


bool 
RouteDbMemory::RouteOperator::matches(const resip::Uri& ruri, 
                                      const resip::Data& method, 
                                      const resip::Data& event)
{
   DebugLog( << "Consider route " << mMatchingPattern 
             << " reqUri=" << ruri
             << " method=" << method 
             << " event=" << event );
   
   if ( !mMethod.empty() )
   {
      if ( mMethod != method) return false;
   }
   if ( !mEvent.empty() )
   {
      if ( mEvent != event) return false;
   }
   if ( !mMatchingPattern.empty() ) //!dcm! -- use regexp
   {
      regex_t preq;
      int ret = regcomp(&preq,mMatchingPattern.c_str(), REG_EXTENDED|REG_NOSUB );
      if ( ret != 0 )
      { 
         ErrLog( << "Routing rule has invalid match expression: " 
                 << mMatchingPattern );
         return false;
      }
      ret = regexec(&preq, ruri.getAor().c_str(), 0, NULL, 0/*eflags*/);
      if ( ret != 0 )
      {
         // did not match 
         return false;
      }
      
      if (mMatchingPattern != ruri.getAor()) return false;
   }
   return true;   
}


resip::Uri 
RouteDbMemory::RouteOperator::transform(const resip::Uri& ruri)
{
   try
   {
      return Uri(mRewriteExpression); //!dcm! -- bogus
   }
   catch( BaseException& e)
   {
      ErrLog( << "Routing rule has invalid transform: " << mRewriteExpression);
      return ruri;
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
