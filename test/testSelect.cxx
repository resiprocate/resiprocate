
#include <cassert>
#include <iostream>

#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/UdpTransport.hxx"

#include <resiprocate/os/Timer.hxx>


using namespace resip;
using namespace std;


int 
main(int argc, char* argv[])
{
   if ( argc != 3 ) 
   {
      cerr << "Usage: testSelect startPort endPort\n";
      exit(1);
   }
   
   int startPort = atoi( argv[1] );
   int endPort = atoi( argv[2] );
   
   cout << "Doing port " << startPort << " to " << endPort << endl;
   
   int fd[0xFFFF];
   
 #define NUMCYCLES 50
   
   UInt64 time1[NUMCYCLES];
   UInt64 time2[NUMCYCLES];
   UInt64 time3[NUMCYCLES];
   
   // open the fd 
   for( int port=startPort; port<endPort; port++)
   {
      fd[port] =  socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if( fd[port] ==  INVALID_SOCKET )
      {
         cerr << "Fail on creating socket for port " << port << endl;
         assert(0);
      }
      
      sockaddr_in addr4; 
      sockaddr* saddr=0;
      int saddrLen = 0;
      
      memset(&addr4, 0, sizeof(addr4));
      addr4.sin_family = AF_INET;
      addr4.sin_port = htons(port);
      addr4.sin_addr.s_addr = htonl(INADDR_ANY); 

      saddr = reinterpret_cast<sockaddr*>(&addr4);
      saddrLen = sizeof(addr4);

      if ( bind( fd[port], saddr, saddrLen) == SOCKET_ERROR )
      {
         assert(0);
      }
      
      bool ok = makeSocketNonBlocking( fd[port] );
      assert(ok);
   }
   

   for ( int i=0; i<NUMCYCLES; i++ )
   {
      FdSet fdset;
      
      time1[i] = Timer::getTimeMicroSec();
      // build the fd
      for( int port=startPort; port<endPort; port++)
      {
         fdset.setRead(fd[port]);
      }
      
      time2[i] = Timer::getTimeMicroSec();
      // select
      int  err = fdset.selectMilliSeconds( 0 );
      assert( err != -1 );
      
      time3[i] = Timer::getTimeMicroSec();
      // do the reads
      for( int port=startPort; port<endPort; port++)
      {
         if (fdset.readyToRead(fd[port]))
         {
            struct sockaddr from;
            
            int MaxBufferSize=1024*4;
            
            char buffer[MaxBufferSize];
            socklen_t fromLen = sizeof(from);
            
            int len = recvfrom( fd[port],
                                buffer,
                                MaxBufferSize,
                                0 /*flags */,
                                (struct sockaddr*)&from,
                                &fromLen);
            
            if ( len == SOCKET_ERROR )
            {
               int err = errno;
               switch (err)
               {
                  case WSANOTINITIALISED:
                     assert(0);
                     break;
                     
                  case EWOULDBLOCK:
                     //cerr << " UdpTransport recvfrom got EWOULDBLOCK";
                     break;
                     
                  case 0:
                     cerr << " UdpTransport recvfrom got error 0 ";
                     break;
                     
                  default:
                     cerr <<"Error receiving, errno="<<err << " " << strerror(err);
                     break;
               }
            }
         }
         
      }
   }
   
   for ( int i=1; i<NUMCYCLES; i++ )
   {
      cout << time2[i] - time1[i] << " ";
      cout << time1[i] - time1[i-1] << " ";
      
      cout << endl;
   }
   
   return 0;
}
