#ifndef SOCKET_H
#define SOCKET_H

#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace p2p
{
  typedef struct sockaddr_in Address;
  typedef int Socket;
  static const Socket INVALID_SOCKET = -1;
  static const int SOCKET_ERROR = -1;
  bool makeSocketNonBlocking(Socket);
  bool makeSocketBlocking(Socket);
  int closeSocket(Socket);
  
  struct AddressHash
   {
      size_t operator()( const struct sockaddr_in sock ) const
      {
         return sock.sin_addr.s_addr ;
      }            
   };
     
   struct AddressCmp
   { 
      bool operator()(const struct sockaddr_in& s1, 
                      const struct sockaddr_in& s2) const
      {
         if ( ( s1.sin_addr.s_addr == s2.sin_addr.s_addr ) &&
              ( s1.sin_port == s2.sin_port) )
         {
            return 1;
         }
         else
         {
            return 0;
         }
      }
   };
}

#endif
