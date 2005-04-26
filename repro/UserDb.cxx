
#include <fcntl.h>
#ifdef WIN32
#include <db_cxx.h>
#else 
#include <db4/db_cxx.h>
#endif
#include <cassert>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/MD5Stream.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TransactionUser.hxx"

#include "repro/UserDb.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

UserDb::UserDb( char* fileName )
{ 
   mDb = new Db( NULL, 0 );
   assert( mDb );

   // if the line bellow seems wrong, you need to check which version 
   // of db you have - it is likely an very out of date version 
   // still trying to figure this out so email fluffy if you have 
   // problems and include your version the DB_VERSION_STRING found 
   // in your db4/db.h file. 
   int ret =mDb->open(NULL,fileName,NULL,DB_BTREE,DB_CREATE,0);
   //int ret =mDb->open(fileName,NULL,DB_BTREE,DB_CREATE,0);

   if ( ret!=0 )
   {
      ErrLog( <<"Could not open user database at " << fileName );
	  assert(0);
   }

   mDb->cursor(NULL,&mCursor,0);
   assert( mCursor );
}


UserDb::~UserDb()
{ 
   assert( mCursor );
   mCursor->close();
   mCursor = 0;
   
   assert( mDb );
   mDb->close(0);
   delete mDb; mDb=0;
}


void 
UserDb::dbWriteRecord( const Data& pKey, const Data& pData )
{ 
   Dbt key( (void*)pKey.data(), (u_int32_t)pKey.size() );
   Dbt data( (void*)pData.data(), (u_int32_t)pData.size() );
   int ret;
   
   assert( mDb );
   ret = mDb->put(NULL,&key,&data,0);
   assert( ret == 0 );

   mDb->sync(0);
}


bool 
UserDb::dbReadRecord( const Data& pKey, Data& pData ) const
{ 
   Dbt key( (void*)pKey.data(), (u_int32_t)pKey.size() );
   Dbt data;
   int ret;
   
   assert( mDb );
   ret = mDb->get(NULL,&key,&data, 0);

   if ( ret == DB_NOTFOUND )
   {
      // key not found 
      return false;
   }
   assert( ret != DB_KEYEMPTY );
   assert( ret == 0 );
   Data result( reinterpret_cast<const char*>(data.get_data()), data.get_size() );
   
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
UserDb::dbRemoveRecord( const Data& pKey )
{ 
   Dbt key( (void*) pKey.data(), (u_int32_t)pKey.size() );

   assert( mDb );
   mDb->del(NULL,&key, 0);
}


resip::Data 
UserDb::dbFirstKey()
{ 
   return dbNextKey( true );
}


resip::Data 
UserDb::dbNextKey(bool first )
{ 
   Dbt key,data;
   int ret;
   
   assert( mDb );
   ret = mCursor->get(&key,&data, first ? DB_FIRST : DB_NEXT);
   if ( ret == DB_NOTFOUND )
   {
      return Data::Empty;
   }
   assert ( ret == 0 );
   
   Data d( reinterpret_cast<const char*>(key.get_data()), key.get_size() );
   
   return d;
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
