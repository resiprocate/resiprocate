#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Errdes.hxx"

#include "repro/ReproVersion.hxx"
#include "repro/HttpBase.hxx"
#include "repro/HttpConnection.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

int HttpConnection::nextPageNumber=1;




HttpConnection::HttpConnection( HttpBase& base, Socket pSock ):
   mHttpBase( base ),
   mPageNumber(nextPageNumber++),
   mSock(pSock),
   mParsedRequest(false)
{
   resip_assert( mSock > 0 );
}


HttpConnection::~HttpConnection()
{
   resip_assert( mSock > 0 );
#ifdef WIN32
   closesocket(mSock); mSock=0;
#else
   close(mSock); mSock=0;
#endif
}

      
void 
HttpConnection::buildFdSet(FdSet& fdset)
{
   if ( !mTxBuffer.empty() )
   {
      fdset.setWrite(mSock);
   }
   fdset.setRead(mSock);
}


bool 
HttpConnection::process(FdSet& fdset)
{
   if ( fdset.hasException(mSock) )
   {
      int errNum = 0;
      int errNumSize = sizeof(errNum);
      getsockopt(mSock,SOL_SOCKET,SO_ERROR,(char *)&errNum,(socklen_t *)&errNumSize);
      InfoLog (<< "Exception reading from socket " 
               << (int)mSock << " code: " << errNum << "; closing connection");
      return false;
   }
   
   if ( fdset.readyToRead( mSock ) )
   {
      bool ok = processSomeReads();
      if ( !ok )
      {
         return false;
      }
   }
   if ( (!mTxBuffer.empty()) && fdset.readyToWrite( mSock ) )
   {
      bool ok = processSomeWrites();
      if ( !ok )
      {
         return false;
      }
   }

   return true;
}


void 
HttpConnection::setPage(const Data& pPage,int response,const Mime& pType)
{
   Data page(pPage);

   switch (response)
   {
      case 401:
      {  
         mTxBuffer += "HTTP/1.0 401 Unauthorized"; mTxBuffer += Symbols::CRLF;
         
         page = ("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
                 "<html><head>"
                 "<title>401 Unauthorized</title>"
                 "</head><body>"
                 "<h1>Unauthorized</h1>"
                 "</body></html>" );
      }
      break;

      case 404:
      {  
         mTxBuffer += "HTTP/1.0 404 Not Found"; mTxBuffer += Symbols::CRLF;
         
         page = ("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
                 "<html><head>"
                 "<title>404 Not Found</title>"
                 "</head><body>"
                 "<h1>Unauthorized</h1>"
                 "</body></html>" );
      }
      break;
      
      case 301:
      {
         mTxBuffer += "HTTP/1.0 301 Moved Permanently"; mTxBuffer += Symbols::CRLF;
         mTxBuffer += "Location: /index.html"; mTxBuffer += Symbols::CRLF;
         
         page = ("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
                 "<html><head>"
                 "<title>301 Moved Permanently</title>"
                 "</head><body>"
                 "<h1>Moved</h1>"
                 "</body></html>" );
      }
      break;
      
      case 200:
      {
         mTxBuffer += "HTTP/1.0 200 OK" ; mTxBuffer += Symbols::CRLF;
      }
      break;

      case 500:
      {
         mTxBuffer += "HTTP/1.0 500 Server failure" ; mTxBuffer += Symbols::CRLF;
      }
      break;

      default:
      {
         resip_assert(0);  

         Data resp;
         { 
            DataStream s(resp);
            s << response;
            s.flush();
         }
         
         mTxBuffer += "HTTP/1.0 ";
         mTxBuffer += resp;
         mTxBuffer += "OK" ; mTxBuffer += Symbols::CRLF;
      }
      break;
   }
   
   Data len;
   {
      DataStream s(len);
      s << page.size();
      s.flush();
   }
    
   mTxBuffer += "WWW-Authenticate: Basic realm=\"";
   if ( mHttpBase.mRealm.empty() )
   {
      mTxBuffer += resip::DnsUtil::getLocalHostName();
   }
   else
   {
      mTxBuffer += mHttpBase.mRealm;
   }
   mTxBuffer += "\" ";
   mTxBuffer += Symbols::CRLF;
 
   mTxBuffer += "Server: Repro Proxy " ; 
   mTxBuffer += Data(VersionUtils::instance().displayVersion());
   mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Mime-version: 1.0 " ; mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Pragma: no-cache " ; mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Content-Length: "; mTxBuffer += len; mTxBuffer += Symbols::CRLF;

   mTxBuffer += "Content-Type: "  ;
   mTxBuffer += pType.type() ;
   mTxBuffer +="/"  ;
   mTxBuffer += pType.subType() ; mTxBuffer += Symbols::CRLF;
   
   mTxBuffer += Symbols::CRLF;
   
   mTxBuffer += page;
}


bool
HttpConnection::processSomeReads()
{
   const int bufSize = 8000;
   char buf[bufSize];
   
 
#if defined(WIN32)
   int bytesRead = ::recv(mSock, buf, bufSize, 0);
#else
   int bytesRead = ::read(mSock, buf, bufSize);
#endif

   if (bytesRead == INVALID_SOCKET)
   {
      int e = getErrno();
      switch (e)
      {
         case EAGAIN:
#if EAGAIN != EWOULDBLOCK
         case EWOULDBLOCK:  // Treat EGAIN and EWOULDBLOCK as the same: http://stackoverflow.com/questions/7003234/which-systems-define-eagain-and-ewouldblock-as-different-values
#endif
            InfoLog (<< "No data ready to read");
            return true;
         case EINTR:
            InfoLog (<< "The call was interrupted by a signal before any data was read.");
            break;
         case EIO:
            InfoLog (<< "I/O error");
            break;
         case EBADF:
            InfoLog (<< "fd is not a valid file descriptor or is not open for reading.");
            break;
         case EINVAL:
            InfoLog (<< "fd is attached to an object which is unsuitable for reading.");
            break;
         case EFAULT:
            InfoLog (<< "buf is outside your accessible address space.");
            break;
         default:
            InfoLog (<< "Some other error");
            break;
      }
      InfoLog (<< "Failed read on " << (int)mSock << " " << errortostringOS(e));
      return false;
   }
   else if (bytesRead == 0)
   {
      InfoLog (<< "Connection closed by remote " );
      return false;
   }

   //DebugLog (<< "HttpConnection::processSomeReads() " 
   //          << " read=" << bytesRead);            

   mRxBuffer += Data( buf, bytesRead );
   
   tryParse();
   
   return true;
}


void 
HttpConnection::tryParse()
{
   //DebugLog (<< "parse " << mRxBuffer );
   
   ParseBuffer pb(mRxBuffer);
   
   // See if we have the entire message
   pb.skipToChars(Symbols::CRLFCRLF);
   if(pb.eof())
   {
      // End of message not found - keep reading
      return;
   }
   pb.reset(pb.start());

   pb.skipToChar(Symbols::SPACE[0]);
   const char* start = pb.skipWhitespace();
   pb.skipToChar(Symbols::SPACE[0]);
   
   if (pb.eof())
   {
      // parse failed - just return 
      return;
   }
   
   Data uri;
   pb.data( uri, start );
 
   DebugLog (<< "parse found URI " << uri );
   mParsedRequest = true;
     
   
   Data user;
   Data password;

   try
   {
      pb.skipToChars("Authorization");
      if (!pb.eof())
      {
         if ( pb.eof() ) DebugLog( << "Did not find Authorization header" );
         pb.skipToChars( "Basic" ); pb.skipN(6);
         if ( pb.eof() ) DebugLog( << "Did not find Authorization basic " );
         pb.skipWhitespace();
         if ( pb.eof() ) DebugLog( << "Something weird in Auhtorization header " );
         if ( !pb.eof() )
         {
            const char* a = pb.position();
            pb.skipNonWhitespace();
            Data buf = pb.data(a);
            
            DebugLog (<< "parse found basic base64 auth data of " << buf );
            Data auth = buf.base64decode();
            
            //DebugLog (<< "parse found basic auth data of " << auth );
            
            ParseBuffer p(auth);
            const char* a1 = p.position();
            p.skipToChar(':');
            user = p.data(a1);
            const char* a2 = p.skipChar(':');
            p.skipToEnd();
            password = p.data(a2);
            
            //DebugLog (<< "parse found basic auth data with user=" << user
            //          << " password=" << password );
         }
      }
   }
   catch ( ... )
   { 
      ErrLog (<< "Some problem finding Authorization header in HTTP request" );
   }
   
   mHttpBase.buildPage(uri,mPageNumber,user,password);
}


bool
HttpConnection::processSomeWrites()
{
   if ( mTxBuffer.empty() )
   {
      return true;
   }
   
   //DebugLog (<< "Writing " << mTxBuffer );

#if defined(WIN32)
   int bytesWritten = ::send( mSock, mTxBuffer.data(), (int)mTxBuffer.size(), 0);
#else
   int bytesWritten = ::write(mSock, mTxBuffer.data(), (int)mTxBuffer.size() );
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      InfoLog (<< "HttpConnection failed write on " << mSock << " " << strerror(e) << " error message from Errdes.hxx file : " << errortostringOS(e) );

      return false;
   }
   
   if (bytesWritten == (int)mTxBuffer.size() )
   {
      DebugLog (<< "Wrote it all" );
      mTxBuffer = Data::Empty;

      return false; // return false causes connection to close and clean up
   }
   else
   {
      Data rest = mTxBuffer.substr(bytesWritten);
      mTxBuffer = rest;
      DebugLog( << "Wrote " << bytesWritten << " bytes - still need to do " << mTxBuffer );
   }
   
   return true;
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
