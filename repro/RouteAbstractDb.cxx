
#include "resiprocate/os/Logger.hxx"

#include "repro/RouteAbstractDb.hxx"

using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

RouteAbstractDb::RouteAbstractDb() 
{
}


RouteAbstractDb::~RouteAbstractDb()
{
}


static void 
encodeString( oDataStream& s, const Data& data )
{
   short len = data.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( data.data(), len );
}


Data 
RouteAbstractDb::serialize( const Route& rec )
{  
   Data data;
   oDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );
   
   assert( rec.mVersion == 1 );
   assert( sizeof( rec.mVersion) == 2 );
   s.write( (char*)(&rec.mVersion) , sizeof( rec.mVersion ) );
   
   encodeString( s, rec.mMethod );
   encodeString( s, rec.mEvent );
   encodeString( s, rec.mMatchingPattern );
   encodeString( s, rec.mRewriteExpression );
   
   s.write( (char*)(&rec.mOrder) , sizeof( rec.mOrder ) );
   
   return data;
}


static Data
decodeString( iDataStream& s)
{
	short len;
	s.read( (char*)(&len), sizeof(len) ); 

	char buf[2048];
	assert( len < 2048 ); // !cj! TODO fix 

	s.read( buf, len );
       
	Data data( buf, len );
	return data;
}

RouteAbstractDb::Route
RouteAbstractDb::deSerialize( const Data& pData )
{  
   RouteAbstractDb::Route rec;
   
   assert( !pData.empty() );

   Data data = pData;
   
   iDataStream s(data);
   short len;
   assert( sizeof(len) == 2 );

   s.read( (char*)(&len), sizeof(len) );
   rec.mVersion = len;
   
   if (  rec.mVersion == 1 )
   {  
         rec.mMethod = decodeString( s );
         rec.mEvent  = decodeString( s );
         rec.mMatchingPattern = decodeString( s );
         rec.mRewriteExpression  = decodeString( s );

         s.read( (char*)(&rec.mOrder), sizeof(rec.mOrder) ); 
   }
   else
   {
      // unkonwn version 
      ErrLog( <<"Data in route database with unknown version " << rec.mVersion );
      assert(0);
   }
      
   return rec;
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
