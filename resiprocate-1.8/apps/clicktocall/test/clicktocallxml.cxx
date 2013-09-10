#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <rutil/XMLCursor.hxx>
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

   if(argc != 6) 
   {
      ErrLog(<< "usage: " << argv[0] <<" <server> <port> <initiator> <destination> <anchor 0|1>");
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
   sd = socket(AF_INET, SOCK_STREAM, 0);
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

   Data request(
      "<ClickToCall>\r\n"
      "  <Request>\r\n"
      "    <Initiator>" + Data(argv[3]) + "</Initiator>\r\n"
      "    <Destination>" + Data(argv[4]) + "</Destination>\r\n"
      "    <AnchorCall>" + Data(argv[5][0] == '0' ? "false" : "true") + "</AnchorCall>\r\n"
      "  </Request>\r\n"
      "</ClickToCall>\r\n");   
   rc = send(sd, request.c_str(), request.size(), 0);
   if(rc < 0) 
   {
      ErrLog(<< "error sending");
      close(sd);
      exit(1);
   }

   char readBuffer[8000];
   while(rc > 0)
   {
      rc = recv(sd, (char*)&readBuffer, sizeof(readBuffer), 0);
      if(rc < 0) 
      {
         ErrLog(<< "error receiving");
         close(sd);
         exit(1);
      }

      if(rc > 0)
      {
         Data response(Data::Borrow, (const char*)&readBuffer, rc);
         InfoLog(<< "Received response: \r\n" << response.c_str());
         ParseBuffer pb(response);
         XMLCursor xml(pb);
         if(xml.firstChild())
         {
            if(xml.nextSibling())
            {
               if(xml.firstChild())
               {
                  do
                  {
                     if(xml.getTag() == "Result")
                     {
                        XMLCursor::AttributeMap attribs = xml.getAttributes();
                        XMLCursor::AttributeMap::iterator it = attribs.find("Code");
                        if(it != attribs.end())
                        {
                           unsigned long statusCode = it->second.convertUnsignedLong();
                           if(statusCode >= 200 && statusCode < 300)
                           {
                              it = attribs.find("Leg");
                              if(it != attribs.end())
                              {
                                 if(it->second == "Destination")
                                 {
                                    // Successfully connected to destination - break out
                                    rc = 0; 
                                 }
                              }
                           }
                           else if(statusCode >=300)
                           {
                              // Failed - break out
                              rc = 0;
                           }
                        }
                        break;
                     }
                  } while(xml.nextSibling());
               }
            }
         }
      }
   }

   //sleepSeconds(5);
   InfoLog(<< "xmlrpcclient done.");
}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

