#include "rutil/ResipAssert.h"
#ifndef WIN32
#include <unistd.h>
#endif
#include "tfm/TelnetClient.hxx"

using namespace std;
using namespace resip;

TelnetClient::TelnetClient(const Data& ip) : mClient(ip.c_str(), 23, Netxx::Timeout(2))
{
   Data init;
   init += (unsigned char)0xFF;
   init += (unsigned char)0xFD;
   init += (unsigned char)0x03;

   init += (unsigned char)0xFF;
   init += (unsigned char)0xFB;
   init += (unsigned char)0x18;

   init += (unsigned char)0xFF;
   init += (unsigned char)0xFB;
   init += (unsigned char)0x20;

   init += (unsigned char)0xFF;
   init += (unsigned char)0xFB;
   init += (unsigned char)0x20;

   send(init, false);
   expect();
   
}

TelnetClient::~TelnetClient()
{
   send(Data("test close"));
   expect();
   send(Data("exit"));
   mClient.close();
}

void
TelnetClient::expect(const Data& expects)
{
   while (mBuffer != expects)
   {
#ifdef WIN32
   Sleep(1000);
#else
   usleep(100000);
#endif
      read();
   }
   //cout << "Matched: " << expects << endl;
}

void
TelnetClient::expect()
{
   read();
   //cout << "Ate: " << mBuffer << endl;
}

void
TelnetClient::send(const Data& data, bool newline)
{
   Data sdata(data);
   if (newline) 
   {
      sdata += Data("\r");
   }
   
   mClient.write(sdata.c_str(), sdata.size()+1);
   //mClient.flush();
}

void
TelnetClient::read()
{
   char buffer[1024];
   Netxx::signed_size_type length=0;
   mBuffer.clear();
   
   // read back the result
   while ( (length = mClient.read(buffer, sizeof(buffer))) > 0)
   {
      //cout << "got " << length << " bytes" << endl;
      buffer[length] = 0;
      for (int i=0; i<length; i++)
      {
         if ((unsigned char)(buffer[i]) == 0xff)
         {
            i+=2;
         }
         else
         {
            //cout << "Appending: '" << buffer[i] << "' " << mBuffer.size() << endl;
            mBuffer += buffer[i];
         }
      }
   }
}

Cisco7960Client::Cisco7960Client(const Data& server, const Data& password) 
   : TelnetClient(server)
{
   static Data intro = ("\r\n"
                        "\r\n"
                        "Password :");
   expect(intro);
   send(password);
   expect();
   send("test open");
   expect();
}

void
Cisco7960Client::onhook()
{
   send("test onhook");
   expect();
}

void
Cisco7960Client::offhook()
{
   send("test offhook");
   expect();
}

void 
Cisco7960Client::selectLine(int lineNumber)
{
   resip_assert(lineNumber >= 1);
   resip_assert(lineNumber <= 6);
   
   send("test key line" + Data(lineNumber));
   expect();
}

void 
Cisco7960Client::dial(const Data& number)
{
   send("test key " + Data(number));
   expect();

}

void 
Cisco7960Client::selectAndDial(int lineNumber, const Data& number)
{
   selectLine(lineNumber);
   dial(number);
}

void
Cisco7960Client::soft(int key, int repeat)
{
   for (int i=0; i<repeat; i++)
   {
      send("test key soft" + Data(key));
      expect();
   }
}

void 
Cisco7960Client::provisionLine(int lineNumber, const Data& name, const Data& authName, const Data& authPass, const Data& proxy, int proxyPort)
{
   resip_assert(0);
}

void 
Cisco7960Client::registerLine(int lineNumber)
{
   send("reg 1 " + Data(lineNumber));
   expect();
}

void 
Cisco7960Client::unregisterLine(int lineNumber)
{
   send("reg 0 " + Data(lineNumber));
   expect();
}

void 
Cisco7960Client::reset()
{
   send("reset");
   expect();
}

void 
Cisco7960Client::reload()
{
   send("erase protflash");
   expect();
}

// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
