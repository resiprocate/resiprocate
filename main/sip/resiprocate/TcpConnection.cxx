#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/TcpConnection.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/Socket.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

TcpConnection::TcpConnection(const Tuple& who, Socket fd) : Connection(who, fd)
{
   DebugLog (<< "Creating TCP connection " << who << " on " << fd);
}

int 
TcpConnection::read( char* buf, int count )
{
   assert(buf);
   assert(count > 0);
   
#if defined(WIN32)
   int bytesRead = ::recv(mSocket, buf, count, 0);
#else
   int bytesRead = ::read(mSocket, buf, count);
#endif
   if (bytesRead == INVALID_SOCKET)
   {
      switch (errno)
      {
         case EAGAIN:
            //InfoLog (<< "No data ready to read");
            return 0;
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

      InfoLog (<< "Failed read on " << mSocket << " " << strerror(errno));
   }

   return bytesRead;
}


int 
TcpConnection::write( const char* buf, const int count )
{
   DebugLog (<< "Writing " << buf);

   assert(buf);
   assert(count > 0);

#if defined(WIN32)
   int bytesWritten = ::send(mSocket, buf, count, 0);
#else
   int bytesWritten = ::write(mSocket, buf, count);
#endif
   if (bytesWritten == INVALID_SOCKET)
   {
      InfoLog (<< "Failed write on " << mSocket << " " << strerror(errno));
   }
   
   return bytesWritten;
}

bool 
TcpConnection::hasDataToRead()
{
   return true;
}

bool 
TcpConnection::isGood()
{
   return true;
}


