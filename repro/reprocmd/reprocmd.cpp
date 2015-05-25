#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <rutil/XMLCursor.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <repro/RegSyncServer.hxx>

using namespace resip;
using namespace std;

int 
main (int argc, char** argv)
{
#ifdef WIN32
   initNetwork();
#endif

   int sd, rc;
   struct sockaddr_in localAddr, servAddr;
   struct hostent *h;

   const char* host;
   short port;
   int cmdIndex;

   // Look for forward slash in args 1 or 3
   if(argc >= 2 && argv[1][0] == '/')
   {
       host = "127.0.0.1";
       port = 5081;
       cmdIndex=1;
   }
   else if(argc >= 4 && argv[3][0] == '/')
   {
      host = argv[1];
      port = (short)atoi(argv[2]);
      cmdIndex = 3;
   }
   else
   {
      cerr << "usage: " << argv[0] <<" [<server> <port>] /<command> [<parm>=<value>]" << endl;
      cerr << "  Valid Commands are:" << endl;
      cerr << "  /GetStackInfo - retrieves low level information about the stack state" << endl;
      cerr << "  /GetStackStats - retrieves a dump of the stack statistics" << endl;
      cerr << "  /ResetStackStats - resets all cumulative stack statistics to zero" << endl;
      cerr << "  /LogDnsCache - causes the DNS cache contents to be written to the resip logs" << endl;
      cerr << "  /ClearDnsCache - empties the stacks DNS cache" << endl;
      cerr << "  /GetDnsCache - retrieves the DNS cache contents" << endl;
      cerr << "  /GetCongestionStats - retrieves the stacks congestion manager stats and state" << endl;
      cerr << "  /SetCongestionTolerance metric=<SIZE|WAIT_TIME|TIME_DEPTH> maxTolerance=<value>" << endl;
      cerr << "                          [fifoDescription=<desc>] - sets congestion tolerances" << endl;
      cerr << "  /Shutdown - signal the proxy to shut down." << endl;
      cerr << "  /Restart - signal the proxy to restart - leaving active registrations in place." << endl;
      cerr << "  /GetProxyConfig - retrieves the all of configuration file settings currently" << endl;
      cerr << "                    being used by the proxy" << endl;
      cerr << "  /AddTransport type=<UDP|TCP|etc.> port=<value> [ipVersion=<V4|V6>]" << endl; 
      cerr << "                [interface=<ipaddress>] [rruri=<AUTO|sip:host:port>]" << endl;
      cerr << "                [udprcvbuflen=<value>] [stun=<YES|NO>] [flags=<uint>]" << endl;
      cerr << "                [domain=<tlsdomainname>] [ssltype=<SSLv23|TLSv1>]" << endl;
      cerr << "                [certfile=<filename>] [keyfile=<filename>]" << endl;
      cerr << "                [tlscvm=<NONE|OPT|MAN>] [tlsuseemail=<YES|NO>]" << endl;
      cerr << "                - adds a new transport to the stack." << endl;
      cerr << "  /RemoveTransport key=<transportKey> - removes the requested transport" << endl; 
      exit(1);
   }

   h = gethostbyname(host);
   if(h==0) 
   {
      cerr << "unknown host " << host << endl;
      exit(1);
   }

   servAddr.sin_family = h->h_addrtype;
   if((unsigned int)h->h_length > sizeof(servAddr.sin_addr.s_addr))
   {
      cerr << "bad h_length" << endl;
      exit(1);
   }
   memcpy((char *) &servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
   servAddr.sin_port = htons(port);
  
   // Create TCP Socket
   sd = (int)socket(AF_INET, SOCK_STREAM, 0);
   if(sd < 0) 
   {
      cerr << "cannot open socket" << endl;
      exit(1);
   }

   // bind to any local interface/port
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   localAddr.sin_port = 0;

   rc = ::bind(sd, (struct sockaddr *) &localAddr, sizeof(localAddr));
   if(rc < 0) 
   {
      cerr <<"error binding locally" << endl;
      exit(1);
   }

   // Connect to server
   rc = ::connect(sd, (struct sockaddr *) &servAddr, sizeof(servAddr));
   if(rc < 0) 
   {
      cerr << "error connecting" << endl;
      exit(1);
   }

   Data command(&argv[cmdIndex][1]); // [1] skips leading slash
   Data request(1024, Data::Preallocate);
   bool dumpRawResult = false;
   if (isEqualNoCase(command, "initialsync"))
   {
      request += "<InitialSync>\r\n";
      request += "  <Request>\r\n";
      request += "     <Version>" + Data(REGSYNC_VERSION) + "</Version>\r\n";
      request += "  </Request>\r\n";
      request += "</InitialSync>\r\n";

      dumpRawResult = true;
   }
   else
   {
      request += "<";
      request += command;
      request += ">\r\n  <Request>\r\n";
      for (int i = cmdIndex + 1; i < argc; i++)
      {
         Data parm;
         Data value;
         Data arg(argv[i]);
         ParseBuffer pb(arg);
         const char *anchor = pb.position();
         pb.skipToChar('=');
         if (!pb.eof())
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
      request += command;
      request += ">\r\n";
   }

   //cout << "Sending:\r\n" << request << endl;

   rc = send(sd, request.c_str(), request.size(), 0);
   if(rc < 0) 
   {
      cerr << "error sending request on socket." << endl;
      closeSocket(sd);
      exit(1);
   }

   char readBuffer[8000];
   while(rc > 0)
   {
      rc = recv(sd, (char*)&readBuffer, sizeof(readBuffer), 0);
      if(rc < 0) 
      {
         cerr << "error receiving response from socket." << endl;
         closeSocket(sd);
         exit(1);
      }

      if(rc > 0)
      {
         Data response(Data::Borrow, (const char*)&readBuffer, rc);

         if (dumpRawResult)
         {
            //cout << "Received response: \r\n" << response.xmlCharDataDecode() << endl;
            cout << "Received response: \r\n" << response << endl;
         }
         else
         {
            ParseBuffer pb(response);
            XMLCursor xml(pb);
            bool responseOK = false;
            if (xml.firstChild() && xml.nextSibling() && xml.firstChild())  // Move to Response node
            {
               while (true)
               {
                  if (isEqualNoCase(xml.getTag(), "Result"))
                  {
                     unsigned int code = 0;
                     Data text;
                     XMLCursor::AttributeMap::const_iterator it = xml.getAttributes().find("Code");
                     if (it != xml.getAttributes().end())
                     {
                        code = it->second.convertUnsignedLong();
                     }
                     if (xml.firstChild())
                     {
                        text = xml.getValue().xmlCharDataDecode();
                        xml.parent();
                     }
                     if (code >= 200 && code < 300)
                     {
                        // Success
                        cout << text << endl;
                     }
                     else
                     {
                        cout << "Error " << code << " processing request: " << text << endl;
                     }
                     responseOK = true;
                  }
                  else if (isEqualNoCase(xml.getTag(), "Data"))
                  {
                     if (xml.firstChild())
                     {
                        cout << xml.getValue().xmlCharDataDecode() << endl;
                        xml.parent();
                     }
                  }
                  if (!xml.nextSibling())
                  {
                     // break on no more sibilings
                     break;
                  }
               }
            }
            if (!responseOK)
            {
               cout << "Unable to parse response:" << endl << response << endl;
            }
            closeSocket(sd);
            break;
         }
      }
   }
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2012
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
/*
 * vi: set shiftwidth=3 expandtab:
 */

