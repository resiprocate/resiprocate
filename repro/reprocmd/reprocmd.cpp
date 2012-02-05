// testXmlRpc.cpp : Defines the entry point for the console application.
//

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <resip/stack/XMLCursor.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

int 
main (int argc, char** argv)
{
#ifdef WIN32
   initNetwork();
#endif

   int sd, rc;
   struct sockaddr_in localAddr, servAddr;
   struct hostent *h;

   if(argc < 4) 
   {
      ErrLog(<< "usage: " << argv[0] <<" <server> <port> <command> [<parm>=<value>]");
      exit(1);
   }

   h = gethostbyname(argv[1]);
   if(h==0) 
   {
      ErrLog(<< "unknown host " << argv[1]);
      exit(1);
   }

   servAddr.sin_family = h->h_addrtype;
   memcpy((char *) &servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
   servAddr.sin_port = htons(atoi(argv[2]));

   // Create TCP Socket
   sd = (int)socket(AF_INET, SOCK_STREAM, 0);
   if(sd < 0) 
   {
      ErrLog(<< "cannot open socket");
      exit(1);
   }

   // bind to any local interface/port
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   localAddr.sin_port = 0;

   rc = bind(sd, (struct sockaddr *) &localAddr, sizeof(localAddr));
   if(rc < 0) 
   {
      ErrLog(<<"error binding locally");
      exit(1);
   }

   // Connect to server
   rc = connect(sd, (struct sockaddr *) &servAddr, sizeof(servAddr));
   if(rc < 0) 
   {
      ErrLog(<< "error connecting");
      exit(1);
   }

   Data request(1024, Data::Preallocate);
   request += "<";
   request += argv[3];
   request += ">\r\n  <Request>\r\n";
   for(int i = 4; i < argc; i++)
   {
      Data parm;
      Data value;
      Data arg(argv[i]);
      ParseBuffer pb(arg);
      const char *anchor = pb.position();
      pb.skipToChar('=');
      if(!pb.eof())
      {
         pb.data(parm, anchor);
         pb.skipChar();
         anchor = pb.position();
         pb.skipToEnd();
         pb.data(value, anchor);
      }
      request += "    <";
      request += parm;
      request += ">";
      request += value;
      request += "</";
      request += parm;
      request += ">\r\n";
   }
   request += "  </Request>\r\n";
   request += "</";
   request += argv[3];
   request += ">\r\n";

   InfoLog( << "Sending:\r\n" << request);

   rc = send(sd, request.c_str(), request.size(), 0);
   if(rc < 0) 
   {
      ErrLog(<< "error sending");
      closeSocket(sd);
      exit(1);
   }

   char readBuffer[8000];
   while(rc > 0)
   {
      rc = recv(sd, (char*)&readBuffer, sizeof(readBuffer), 0);
      if(rc < 0) 
      {
         ErrLog(<< "error receiving");
         closeSocket(sd);
         exit(1);
      }

      if(rc > 0)
      {
         Data response(Data::Borrow, (const char*)&readBuffer, rc);
         InfoLog(<< "Received response: \r\n" << response.xmlCharDataDecode());

         closeSocket(sd); 
         break;
      }
   }

   //sleepSeconds(5);
   InfoLog(<< "reprocmd done.");
}
