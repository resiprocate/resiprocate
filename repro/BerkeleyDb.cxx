#ifdef HAVE_CONFIG_H
#include <config.hxx>
#endif

#include <fcntl.h>
#include <cassert>

#include "rutil/WinLeakCheck.hxx"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/FileSystem.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

BerkeleyDb::BerkeleyDb()
{
   init(Data::Empty, Data::Empty);
}


BerkeleyDb::BerkeleyDb( const Data& dbPath, const Data& dbName )
{
   init(dbPath, dbName);
}


void
BerkeleyDb::init( const Data& dbPath, const Data& dbName )
{ 
   Data filePath(dbPath);

   // An empty path is how you specify the current working directory as a path
   if ( !filePath.empty() )
   {
      filePath += FileSystem::PathSeparator;
   }

   if ( dbName.empty() )
   {
      DebugLog( << "No BerkeleyDb prefix specified - using default" );
      filePath += "repro";
   }
   else
   {
      filePath += dbName;
   }

   InfoLog( << "Using BerkeleyDb prefixed with " << filePath );

   sane = true;
   
   assert( MaxTable <= 5 );
   
   for (int i=0;i<MaxTable;i++)
   {
      mDb[i] = new Db( NULL, 0 );
      assert( mDb[i] );
      
      // if the line bellow seems wrong, you need to check which version 
      // of db you have - it is likely an very out of date version 
      // still trying to figure this out so email fluffy if you have 
      // problems and include your version the DB_VERSION_STRING found 
      // in your db4/db.h file. 
      Data fileName( filePath );
      switch (i)
      {
         case UserTable:
            fileName += "_user.db"; break;
         case RouteTable:
            fileName += "_route.db"; break;
         case AclTable:
            fileName += "_acl.db"; break;
         case ConfigTable:
            fileName += "_config.db"; break;
         case ParametersTable:
            fileName += "_parameters.db"; break;
         default:
            assert(0);
      }
      
      DebugLog( << "About to open Berkeley DB: " << fileName );
      int ret =mDb[i]->open(NULL,fileName.c_str(),NULL,DB_BTREE,DB_CREATE | DB_THREAD,0);
      //int ret =mDb->open(fileName,NULL,DB_BTREE,DB_CREATE,0);
      
      if ( ret!=0 )
      {
         ErrLog( <<"Could not open user database at " << fileName );
         sane = false;
         return;
      }
      DebugLog( << "Opened Berkeley DB: " << fileName );
      
      mDb[i]->cursor(NULL,&mCursor[i],0);
      assert( mCursor );
   }
}


BerkeleyDb::~BerkeleyDb()
{  
   for (int i=0;i<MaxTable;i++)
   {
      assert( mCursor[i] );
      mCursor[i]->close();
      mCursor[i] = 0;
      
      assert( mDb[i] );
      mDb[i]->close(0);
      delete mDb[i]; mDb[i]=0;
   }
}


void 
BerkeleyDb::dbWriteRecord( const Table table, 
                          const resip::Data& pKey, 
                          const resip::Data& pData )
{
   Dbt key( (void*)pKey.data(), (::u_int32_t)pKey.size() );
   Dbt data( (void*)pData.data(), (::u_int32_t)pData.size() );
   int ret;
   
   assert( mDb );
   ret = mDb[table]->put(NULL,&key,&data,0);
   assert( ret == 0 );

   mDb[table]->sync(0);
}


bool 
BerkeleyDb::dbReadRecord( const Table table, 
                         const resip::Data& pKey, 
                         resip::Data& pData ) const
{ 
   Dbt key( (void*)pKey.data(), (::u_int32_t)pKey.size() );
   Dbt data;
   data.set_flags( DB_DBT_MALLOC );

   int ret;
   
   assert( mDb );
   ret = mDb[table]->get(NULL,&key,&data, 0);

   if ( ret == DB_NOTFOUND )
   {
      // key not found 
      if ( data.get_data() )      
         free( data.get_data() );
      return false;
   }
   assert( ret != DB_KEYEMPTY );
   assert( ret == 0 );
   Data result( reinterpret_cast<const char*>(data.get_data()), data.get_size() );
   if ( data.get_data() )      
      free( data.get_data() );
   if (result.empty())
   {
      // this should never happen
      return false;
   }
   
   assert( !result.empty() );
   pData = result;
   
   return true;
}


void 
BerkeleyDb::dbEraseRecord( const Table table, 
                          const resip::Data& pKey )
{ 
   Dbt key( (void*) pKey.data(), (::u_int32_t)pKey.size() );

   assert( mDb );
   mDb[table]->del(NULL,&key, 0);

   mDb[table]->sync(0);
}


resip::Data 
BerkeleyDb::dbNextKey( const Table table, 
                      bool first)
{ 
   Dbt key,data;
   int ret;
   
   assert( mDb );
   ret = mCursor[table]->get(&key,&data, first ? DB_FIRST : DB_NEXT);
   if ( ret == DB_NOTFOUND )
   {
      return Data::Empty;
   }
   assert ( ret == 0 );
   
   Data d( reinterpret_cast<const char*>(key.get_data()), key.get_size() );
   
   return d;
}

bool
BerkeleyDb::isSane()
{
  return sane;
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
