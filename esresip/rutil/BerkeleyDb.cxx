/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifdef HAVE_CONFIG_H
#include <repro/config.hxx>
#endif

#include <fcntl.h>
#include <cassert>

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/EsLogger.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

BerkeleyDb::BerkeleyDb( )
{ 
   init("repro_bdb.db");
}



BerkeleyDb::BerkeleyDb( const resip::Data& dbName )
{ 
   init(dbName);
}

bool
BerkeleyBd::init(const resip::Data& dbName)
{
   InfoLog( << "Using BerkeleyDb " << dbName );
   sane=true;
   
   mDb = new Db( NULL, 0 );
   assert( mDb );
   
   // if the line bellow seems wrong, you need to check which version 
   // of db you have - it is likely an very out of date version 
   // still trying to figure this out so email fluffy if you have 
   // problems and include your version the DB_VERSION_STRING found 
   // in your db4/db.h file. 
   Data fileName( dbName );
   
   DebugLog( << "About to open Berkeley DB: " << fileName );
   int ret =mDb->open(NULL,fileName.c_str(),NULL,DB_BTREE,DB_CREATE,0);
   //int ret =mDb->open(fileName,NULL,DB_BTREE,DB_CREATE,0);
   
   if ( ret!=0 )
   {
      ErrLog( <<"Could not open user database at " << fileName );
      sane = false;
      return;
   }

   DebugLog( << "Opened Berkeley DB: " << fileName );

   
   mDb->cursor(NULL,&mCursor,0);
   assert( mCursor );
   mName=dbName;

}

BerkeleyDb::~BerkeleyDb()
{  
      assert( mCursor );
      mCursor->close();
      mCursor = 0;
      
      assert( mDb );
      mDb->close(0);
      delete mDb; mDb=0;
}


bool 
BerkeleyDb::dbWriteRecord(const resip::Data& pKey, 
                          const resip::Data& pData )
{
   if(pKey.empty())
   {
      return false;
   }

   if( !mDb )
   {
      ES_FATAL("es.repro.berkeleybackend","The database pointer in Berkeley Db was never initialized! "
                    <<"(Maybe the constructor for AbstractDb was called instead?)");
      ES_FATAL("es.repro.warning","A database pointer in BerkeleyDb was not initialized. "
                    << "The name was " << mName << "." );
      return false;
   }
   
   Dbt key( (void*)pKey.data(), (::u_int32_t)pKey.size() );
   Dbt data( (void*)pData.data(), (::u_int32_t)pData.size() );
   int ret;
   
   
   ret = mDb->put(NULL,&key,&data,0);
   if( ret != 0 )
   {
      ES_ERROR("es.repro.berkeleybackend", "Write failed in dbWriteRecord. Someone should give this a closer look.");
      return false;
   }
   mDb->sync(0);
   return true;
}


bool 
BerkeleyDb::dbReadRecord(const resip::Data& pKey, 
                         resip::Data& pData ) const
{ 

   if(pKey.empty())
   {
      pData="";
      return false;
   }
   
   if( !mDb )
   {
      ES_FATAL("es.repro.berkeleybackend","The database pointer in Berkeley Db was never initialized! "
                    <<"(Maybe the constructor for AbstractDb was called instead?)");
      ES_FATAL("es.repro.warning","A database pointer in BerkeleyDb was not initialized. "
                    << "The name was " << mName << "." );
      pData="";
      return false;
   }
   
   Dbt key( (void*)pKey.data(), (::u_int32_t)pKey.size() );
   Dbt data;
   int ret;
   
   ret = mDb->get(NULL,&key,&data, 0);

   if( ret != 0 )
   {
      pData="";
      return false;
   }

   Data result( reinterpret_cast<const char*>(data.get_data()), data.get_size() );
   
   if (result.empty())
   {
      pData=result;
      return false;
   }
   
   pData = result;
   
   return true;
}


bool 
BerkeleyDb::dbEraseRecord(const resip::Data& pKey )
{ 
   if(pKey.empty())
   {
      return false;
   }
   
   if( !mDb )
   {
      ES_FATAL("es.repro.berkeleybackend","The database pointer in Berkeley Db was never initialized! "
                    <<"(Maybe the constructor for AbstractDb was called instead?)");
      ES_FATAL("es.repro.warning","A database pointer in BerkeleyDb was not initialized. "
                    << "The name was " << mName << "." );
      return false;
   }
   
   Dbt key( (void*) pKey.data(), (::u_int32_t)pKey.size() );

   mDb->del(NULL,&key, 0);

   mDb->sync(0);
   return true;
}


void 
BerkeleyDb::dbNextKey(bool first,resip::Data& key)
{
   key.clear();
   if( !mDb )
   {
      ES_FATAL("es.repro.berkeleybackend","The database pointer in Berkeley Db was never initialized! "
                    <<"(Maybe the constructor for AbstractDb was called instead?)");
      ES_FATAL("es.repro.warning","A database pointer in BerkeleyDb was not initialized. "
                    << "The name was " << mName << "." );
      return;
   }
   
   Dbt prekey,data;
   int ret;
   
   ret = mCursor->get(&key,&data, first ? DB_FIRST : DB_NEXT);

   if ( ret != 0 )
   {
      return;
   }
   
   key.append( reinterpret_cast<const char*>(key.get_data()), key.get_size() );
}

void
BerkeleyDb::getKeys(std::vector<resip::Data>& container)
{
   resip::WriteLock g(mCursorMutex);
   //!bwc! The cursor used in this function is not exactly threadsafe.
   //If another call alters the underlying database, the cursor can
   //be left hanging. So we would like to prevent any write
   //from being executed while this function is working,
   //but prevent those write calls from locking each other out
   //(since they are threadsafe wrt each other).
   //Read/write locks will accomplish this (one "writer", or multiple
   //"readers", but not both), despite the fact that this "writer" is a
   //a read call, and the "readers" are in fact threadsafe write calls.
   //Yes, this is odd. I will refactor this to solve the problem eventually.
   
  Data key;
  dbNextKey(true,key);
  
  while(key != Data::Empty)
  {
      ES_DEBUG("es.repro.provisioning","In BerkeleyDb::getKeys(), found key " << key);
      container.push_back(key);
      dbNextKey(false,key);
  }
  
  ES_DEBUG("es.repro.provisioning","In BerkeleyDb::getKeys(), number of keys found is " << container.size());
   
}

bool
BerkeleyDb::isSane()
{
  return sane;
}


/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
