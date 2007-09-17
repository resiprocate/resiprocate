#include "Socket.h"
using namespace std;
using namespace p2p;

bool p2p::makeSocketNonBlocking(Socket fd)
{
  int flags  = fcntl( fd, F_GETFL, 0);
  int errNoBlock = fcntl(fd, F_SETFL, flags | O_NONBLOCK );
  if ( errNoBlock != 0 ) // !cj! I may have messed up this line 
    {
      return false;
    }
  return true;
}

bool p2p::makeSocketBlocking(Socket fd)
{
  int flags  = fcntl( fd, F_GETFL, 0);
  int errNoBlock = fcntl(fd, F_SETFL, flags & ~O_NONBLOCK );
  if ( errNoBlock != 0 ) // !cj! I may have messed up this line 
    {
      return false;
    }
  return true;
}


int p2p::closeSocket(Socket fd)
{
   int ret = ::close(fd);
   if (ret < 0)
   {
     cout << "FAILED TO CLOSE SOCKET" << endl;
   }
   return ret;
}
