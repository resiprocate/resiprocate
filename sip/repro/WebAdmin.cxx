
#include <cassert>

#include "repro/WebAdmin.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/TransportType.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

class HttpConnection;

class HttpBase
{
      friend class HttpConnection;
      
   public:
      HttpBase( int port, IpVersion version);
      virtual ~HttpBase();
      
      void buildFdSet(FdSet& fdset);
      void process(FdSet& fdset);

   protected:
      virtual void buildPage( const Data& uri, int pageNumber )=0;
      void setPage( const Data& page, int pageNumber );
      
   private:
      static const int MaxConnections = 10;
      
      Socket mFd;
      int nextConnection;
      Tuple mTuple;

      HttpConnection* mConnection[MaxConnections];
};


class HttpConnection
{
      friend class HttpBase;
      
   public:
      HttpConnection( HttpBase& webAdmin, Socket pSock );
      ~HttpConnection();
      
      void buildFdSet(FdSet& fdset);
      bool process(FdSet& fdset);

      void setPage(const Data& page);

   private:
      bool processSomeReads();
      bool processSomeWrites();
      void tryParse();
            
      HttpBase& mHttpBase;
      const int mPageNumber;
      static int nextPageNumber;
            
      Socket mSock;
      Data mRxBuffer;
      Data mTxBuffer;
      bool mParsedRequest;
};


class WebAdmin: public HttpBase
{
   public:
      WebAdmin( int port, IpVersion version=V4);
      
   protected:
      virtual void buildPage( const Data& uri, int pageNumber );

   private: 
      resip::Data buildUserPage();
};


int HttpConnection::nextPageNumber=1;


#if 0
int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cerr, Log::Err, argv[0]);
   Log::setLevel(Log::Debug);

   WebAdmin web( 5080 );
   
   while (1)
   {
      FdSet fdset; 
     
      web.buildFdSet(fdset);
      fdset.selectMilliSeconds( 10*1000 );

      web.process(fdset);
   }
   
}
#endif


HttpBase::~HttpBase()
{
   close(mFd); mFd=0;
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         delete mConnection[i] ; mConnection[i]=0;
      }
   }
}



HttpBase::HttpBase( int port, IpVersion ipVer):
   nextConnection(0),
   mTuple(Data::Empty,port,ipVer,TCP,Data::Empty)
{
   assert( ipVer == V4 );
   
   for ( int i=0 ; i<MaxConnections; i++)
   {
      mConnection[i]=0;
   }
   
#ifdef USE_IPV6
   mFd = ::socket(ipVer == V4 ? PF_INET : PF_INET6, SOCK_STREAM, 0);
#else
   mFd = ::socket(PF_INET, SOCK_STREAM, 0);
#endif
   
   if ( mFd == INVALID_SOCKET )
   {
      int e = getErrno();
      InfoLog (<< "Failed to create socket: " << strerror(e));
      assert(0); // TODO 
      //throw Exception("Can't create HttpBase listner socket", __FILE__,__LINE__);
   }

   DebugLog (<< "Creating fd=" << mFd 
             << (ipVer == V4 ? " V4/" : " V6/") );
      
#if !defined(WIN32)
   int on = 1;
   if ( ::setsockopt ( mFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) )
   {
      int e = getErrno();
      InfoLog (<< "Couldn't set sockoptions SO_REUSEPORT | SO_REUSEADDR: " << strerror(e));
      assert(0); //throw Exception("Failed setsockopt", __FILE__,__LINE__);
   }
#endif
   
   DebugLog (<< "Binding to " << DnsUtil::inet_ntop(mTuple));
   
   if ( ::bind( mFd, &mTuple.getMutableSockaddr(), mTuple.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         ErrLog (<< mTuple << " already in use ");
         assert(0); // throw Transport::Exception("port already in use", __FILE__,__LINE__);
      }
      else
      {
         ErrLog (<< "Could not bind to " << mTuple);
         assert(0); //throw Transport::Exception("Could not use port", __FILE__,__LINE__);
      }
   }
   
   bool ok = makeSocketNonBlocking(mFd);
   if ( !ok )
   {
      ErrLog (<< "Could not make HTTP socket non-blocking " << port );
       assert(0); // tthrow Transport::Exception("Failed making socket non-blocking", __FILE__,__LINE__);
   }
   
   // do the listen, seting the maximum queue size for compeletly established
   // sockets -- on linux, tcp_max_syn_backlog should be used for the incomplete
   // queue size(see man listen)
   int e = listen(mFd,5 );

   if (e != 0 )
   {
      int e = getErrno();
      InfoLog (<< "Failed listen " << strerror(e));
      // !cj! deal with errors
	   assert(0); // tthrow Transport::Exception("Address already in use", __FILE__,__LINE__);
   }
}


void 
HttpBase::buildFdSet(FdSet& fdset)
{ 
   fdset.setRead( mFd );
   
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         mConnection[i]->buildFdSet(fdset);
      }
   }
}


void 
HttpBase::process(FdSet& fdset)
{
   if (fdset.readyToRead(mFd))
   {
      Tuple tuple(mTuple);
      struct sockaddr& peer = tuple.getMutableSockaddr();
      socklen_t peerLen = tuple.length();
      Socket sock = accept( mFd, &peer, &peerLen);
      if ( sock == SOCKET_ERROR )
      {
         int e = getErrno();
         switch (e)
         {
            case EWOULDBLOCK:
               // !jf! this can not be ready in some cases 
               return;
            default:
               assert(0); // Transport::error(e);
         }
         return;
      }
      makeSocketNonBlocking(sock);
      
      int c = nextConnection;
      nextConnection = ( nextConnection+1 ) % MaxConnections;
      
      if ( mConnection[c] )
      {
         delete mConnection[c]; mConnection[c] = 0;
      }
      
      mConnection[c] = new HttpConnection(*this,sock);
      
      DebugLog (<< "Received TCP connection as connection=" << c << " fd=" << sock);
   }
    
   for( int i=0; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         bool ok = mConnection[i]->process(fdset);
         if ( !ok )
         {
            delete mConnection[i]; mConnection[i]=0;
         }
      }
   }
}


void HttpBase::setPage( const Data& page, int pageNumber )
{
   for ( int i=0 ; i<MaxConnections; i++)
   {
      if ( mConnection[i] )
      {
         if ( mConnection[i]->mPageNumber == pageNumber )
         {
            mConnection[i]->setPage( page );
         }
      }
   }
}


HttpConnection::HttpConnection( HttpBase& base, Socket pSock ):
   mHttpBase( base ),
   mPageNumber(nextPageNumber++),
   mSock(pSock),
   mParsedRequest(false)
{
}


HttpConnection::~HttpConnection()
{
   close(mSock); mSock=0;
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
      InfoLog (<< "Exception reading from socket " << mSock << " code: " << errNum << "; closing connection");
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
HttpConnection::setPage(const Data& page)
{
   Data len;
   {
      DataStream s(len);
      s << page.size();
      s.flush();
   }
      
   mTxBuffer += "HTTP/1.1 200 OK" ; mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Server: Repro Proxy " ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "Date: Fri, 01 Apr 2005 08:08:15 GMT" ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "Last-Modified: Fri, 01 Apr 2005 08:07:15 GMT" ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "ETag: \"4c622d-8ee-424d0133\" " ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "Accept-Ranges: bytes" ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "Keep-Alive: timeout=15, max=100" ; mTxBuffer += Symbols::CRLF;
   //mTxBuffer += "Connection: Keep-Alive" ; mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Content-Length: "; mTxBuffer += len; mTxBuffer += Symbols::CRLF;
   mTxBuffer += "Content-Type: text/html" ; mTxBuffer += Symbols::CRLF;
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
      InfoLog (<< "Failed read on " << mSock << " " << strerror(e));
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
  
   mHttpBase.buildPage(uri,mPageNumber);
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
   int bytesWritten = ::send( mSock, mTxBuffer.data(), mTxBuffer.size(), 0);
#else
   int bytesWritten = ::write(mSock, mTxBuffer.data(), mTxBuffer.size() );
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      InfoLog (<< "HttpConnection failed write on " << mSock << " " << strerror(e));
      return false;
   }
   
   if (bytesWritten == (int)mTxBuffer.size() )
   {
      DebugLog (<< "Wrote it all" );
      mTxBuffer = Data::Empty;
   }
   else
   {
      Data rest = mTxBuffer.substr(bytesWritten);
      mTxBuffer = rest;
      DebugLog( << "Wrote " << bytesWritten << " bytes - still need to do " << mTxBuffer );
   }
   
   return true;
}


WebAdmin::WebAdmin( int port, IpVersion version):
   HttpBase( port, version )
{
}


void 
WebAdmin::buildPage( const Data& uri, int pageNumber )
{
   ParseBuffer pb(uri);
   
   const char* anchor = pb.skipChar('/');
   pb.skipToChar('?');
   Data pageName;
   pb.data(pageName,anchor);
   
   if (pb.eof())
   {
         DebugLog (<< "got page name " << pageName );
   }
   else
   {
      pb.skipChar('?');

      while ( !pb.eof() )
      {
           const char* anchor1 = pb.position();
           pb.skipToChar('=');
           Data key;
           pb.data(key,anchor1);
 
           const char* anchor2 = pb.skipChar('=');
           pb.skipToChar('&');
           Data value;
           pb.data(value,anchor2); 
           
           if ( !pb.eof() )
           {
              pb.skipChar('&');
           }
           
           DebugLog (<< "key=" << key << " value=" << value );
      }
   }
   
   Data page = buildUserPage();
   setPage( page, pageNumber );
}
  

Data
WebAdmin::buildUserPage()
{
   Data ret;
   {
      DataStream s(ret);

      s << "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
""
"<html xmlns=\"http://www.w3.org/1999/xhtml\">"
""
"	<head>"
"		<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />"
"		<title>Repo Proxy</title>"
"	</head>"
""
"	<body bgcolor=\"#ffffff\">"
"		<table width=\"100%\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
"			<tr>"
"				<td>"
"					<h1>Repro Proxy</h1>"
"				</td>"
"			</tr>"
"			<tr>"
"				<td>"
"					<table width=\"95%\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
"						<tr>"
"							<td>"
"								<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
"									<tr>"
"										<td>"
"											<h3>Add User</h3>"
"										</td>"
"									</tr>"
"									<tr>"
"										<td>"
"											<h3>Users</h3>"
"										</td>"
"									</tr>"
"									<tr>"
"										<td>"
"											<h3>Config</h3>"
"										</td>"
"									</tr>"
"									<tr>"
"										<td>"
"											<h3>Stats</h3>"
"										</td>"
"									</tr>"
"								</table>"
"							</td>"
"							<td align=\"left\" valign=\"top\" width=\"85%\">"
"								<form id=\"addUserForm\" action=\"\" method=\"get\" name=\"addUserForm\" enctype=\"application/x-www-form-urlencoded\">"
"									<table width=\"90%\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
"										<tr>"
"											<td align=\"right\" valign=\"middle\" width=\"30%\">User Name:</td>"
"											<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" size=\"24\"/></td>"
"										</tr>"
"										<tr>"
"											<td align=\"right\" valign=\"middle\" width=\"30%\">Password:</td>"
"											<td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"24\"/></td>"
"										</tr>"
"										<tr>"
"											<td align=\"right\" valign=\"middle\" width=\"30%\">Full Name:</td>"
"											<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" size=\"24\"/></td>"
"										</tr>"
"										<tr>"
"											<td align=\"right\" valign=\"middle\" width=\"30%\">Email:</td>"
"											<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" size=\"24\"/></td>"
"										</tr>"
"									</table>"
"									<input type=\"reset\" value=\"Reset\"/>"
"								<input type=\"submit\" name=\"submit\" value=\"OK\"/>"
"								</form>"
"								</td>"
"						</tr>"
"					</table>"
"				</td>"
"			</tr>"
"		</table>"
"	</body>"
""
"</html>";
      
      s.flush();
   }
   return ret;
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
