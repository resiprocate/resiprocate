
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
   if ( argc != 4 ) 
   {
      cerr << "Usage: testSelect startPort endPort rate\n";
      exit(1);
   }
   
   int startPort = atoi( argv[1] );
   int endPort = atoi( argv[2] );
   
   cout << "Doing port " << startPort << " to " << endPort << endl;
   
   int fd[0xFFFF];
   
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
   

   while (1)
   {
      FdSet fdset;
       
      UInt64 t = Timer::getTimeMicroSec();

      for( int port=startPort; port<endPort; port++)
      {
         if (fdset.readyToRead(fd[port]))
         {
            struct sockaddr from;
            
            int MaxBufferSize=1024*4;
            
            char buffer[MaxBufferSize];
            socklen_t fromLen = sizeof(from);
            
            sockaddr_in addrin;
            addrin.sin_addr = sendData->destination.ipv4;
            addrin.sin_port = htons(port);
            addrin.sin_family = AF_INET;
            
            int len = sendto( fd[port],
                                buffer,
                                MaxBufferSize,
                                0, // flags
                                (const sockaddr*)&addrin, sizeof(sockaddr_in) );

            if ( len == SOCKET_ERROR )
            {
               assert(0);
            }
            if ( len != MaxBufferSize )
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
   
   return 0;
}
