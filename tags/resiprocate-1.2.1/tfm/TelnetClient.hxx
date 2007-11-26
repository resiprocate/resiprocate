#if !defined(Dragon_Telnet_Client_hxx)
#define Dragon_Telnet_Client_hxx

#include <Netxx/Stream.h>
#include <Netxx/Timeout.h>

#include "rutil/Data.hxx"

class TelnetClient
{
   public:
      TelnetClient(const resip::Data& server);
      virtual ~TelnetClient();

   protected:
      void read();
      void expect(const resip::Data& expects);
      void expect();
      void send(const resip::Data& data, bool newline=true);
      
   private:
      Netxx::Stream mClient;
      resip::Data mBuffer;
};
 
class Cisco7960Client : public TelnetClient
{
   public:
      // login, open a test session
      Cisco7960Client(const resip::Data& server, const resip::Data& password);
      
      void onhook();
      void offhook();
      void selectLine(int lineNumber);
      void soft(int key, int repeat = 1);
      void provisionLine(int lineNumber, 
                         const resip::Data& name, 
                         const resip::Data& authName, 
                         const resip::Data& authPass, 
                         const resip::Data& proxy, 
                         int proxyPort=5060);
      void dial(const resip::Data& number);
      void selectAndDial(int lineNumber, const resip::Data& number);
      void registerLine(int lineNumber);
      void unregisterLine(int lineNumber);
      void reset();  // reboots
      void reload(); // reloads configuration
};

#endif

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
