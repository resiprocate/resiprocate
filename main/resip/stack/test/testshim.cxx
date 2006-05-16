#include "dtls_shim.h"
#include "resip/stack/Security.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

int openSocket(int port);
bool 
getMessage( int fd, 
            unsigned char* buf, int* len,
            sockaddr& source);
bool 
sendMessage( int fd, 
             unsigned char* buf, int l, 
             const sockaddr& dest);

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, Log::Info, argv[0]);
   
   if (argc == 0)
   {
      cerr << "testshim myport myaor [targetip targetport]" << endl;
      exit(0);
   }
   
   int myPort = atoi(argv[1]);
   char* aorp = argv[2];
   char* target = 0;
   if (argc > 3)
   {
      target = argv[3];
      int remotePort = atoi(argv[4]);
   }
   
   resip::Security sec("~/.sipCerts");
   sec.preload();

   resip::Data aor(aorp);
   sec.generateUserCert(aor);
   X509* cert = sec.getUserCert(aor);
   InfoLog (<< "Generated user cert for " << aor);
   
   dtls_shim_h shim = dtls_shim_init(cert, 0);
   InfoLog (<< "DTLS shim initialized");
   
   if (result == 0)
   {
      exit(-1);
   }
   assert(dtls_shim_get_client_data(shim) == 0);

   
   dtls_shim_con_info_s targetc;
   targetc.remote.sin_family = AF_INET;
   resip::DnsUtil::inet_pton(target, targetc.remote.sin_addr);
   targetc.remote.sin_port = htons(remotePort);

   resip::Socket socket = openSocket(myPort, 0, false);
   makeSocketNonBlocking(socket);
   
   // This kicks off the handshake
   if (target)
   {
      dtls_shim_iostatus_e status;
      unsigned char obuf[4096];
      int bytes = dtls_shim_write(shim, targetc, obuf, sizeof(obuf), 0, 0, &status);
      switch (status)
      {
         case DTLS_SHIM_WANT_WRITE:
            sendMessage(source, obuf, bytes, targetc.remote);
            break;

         case DTLS_SHIM_OK:
         case DTLS_SHIM_WANT_READ:
            break;
      }
   }
   
   while (1)
   {
      FdSet fdset;
      fdset.setRead(socket);
      
      if (fdset.selectMilliSeconds(100) >= 0)
      {
         if (fdset.readyToRead(socket))
         {
            char buffer[4096];
            unsigned int len = sizeof(buffer);
            dtls_shim_con_info_s source;
            if (getMessage(socket, buffer, &len, source.remote))
            {
               unsigned char obuf[4096];
               dtls_shim_iostatus_e status;
               int bytes = dtls_shim_read(shim, source, obuf, sizeof(obuf), buffer, len, &status);
               switch (status)
               {
                  case DTLS_SHIM_WANT_WRITE:
                     sendMessage(source, obuf, bytes, targetc.remote);
                     break;

                  case DTLS_SHIM_OK:
                  case DTLS_SHIM_WANT_READ:
                     break;
               }
            }
         }
      }
   }
      
   dtls_shim_fini(shim);
   
}

int openSocket(int port)
{
   int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == -1 )
   {
      return -1;
   }
    
   struct sockaddr_in addr;
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
   
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
      int e = errno;
      switch (e)
      {
         case 0:
            clog << "Could not bind socket" << endl;
            ::close(fd);
            return -1;
         case EADDRINUSE:
            clog << "Port " << port << " for receiving UDP is in use" << endl;
            ::close(fd);
            return -1;
         case EADDRNOTAVAIL:
            clog << "Cannot assign requested address" << endl;
            ::close(fd);
            return -1;
            break;
         default:
            clog << "Could not bind UDP receive port"
                 << "Error=" << e << " " << strerror(e) << endl;
            ::close(fd);
            return -1;
      }
   }
   return fd;
}

bool 
getMessage( int fd, 
            unsigned char* buf, int* len,
            sockaddr& source)
{
   assert( fd != -1 );
	
   int originalSize = *len;
   assert( originalSize > 0 );
   
   int fromLen = sizeof(source);
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&source,
                   (socklen_t*)&fromLen);
	
   if ( *len == -1 )
   {
      int err = errno;
      switch (err)
      {
         case ENOTSOCK:
            clog << "Error fd not a socket" <<   endl;
            break;
         case ECONNRESET:
            clog << "Error connection reset - host not reachable" <<   endl;
            break;
				
         default:
            clog << "Socket Error=" << err << endl;
      }
      return false;
   }
   
   if ( *len < 0 )
   {
      clog << "socket closed? negative len" << endl;
      return false;
   }
    
   if ( *len == 0 )
   {
      clog << "socket closed? zero len" << endl;
      return false;
   }
    
   if ( (*len)+1 >= originalSize )
   {
      clog << "Received a message that was too large" << endl;
      return false;
   }
   return true;
}

bool 
sendMessage( int fd, 
             unsigned char* buf, int l, 
             const sockaddr& dest)
{
   assert( fd != -1 );
   s = sendto(fd, buf, l, 0, &dest, sizeof(dest));
   if ( s == SOCKET_ERROR )
   {
      int e = errno;
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            clog << "err EAFNOSUPPORT in send" << endl;
         }
         break;
         default:
         {
            clog << "err " << e << " "  << strerror(e) << " in send" << endl;
         }
      }
      return false;
   }
    
   if ( s == 0 )
   {
      clog << "no data sent in send" << endl;
      return false;
   }
    
   if ( s != l )
   {
      clog << "only " << s << " out of " << l << " bytes sent" << endl;
      return false;
   }
    
   return true;
}
