#ifndef TransportDriver_hxx
#define TransportDriver_hxx

//#include "resip/sip/SipStack.hxx"
#include "resip/sip/TimerQueue.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Mutex.hxx"

#include <sys/types.h>
#include <boost/shared_ptr.hpp>
#include <loki/Singleton.h>

//#ifndef WIN32
//#include <sys/select.h> // posix
//#endif


class TransportDriverImpl : public resip::ThreadIf
{
   public:
      class Client
      {
         public:
            virtual void process(resip::FdSet& fdset) = 0;
            virtual void registerWithTransportDriver();
            virtual void unregisterFromTransportDriver();
            virtual void buildFdSet(resip::FdSet& fdset) = 0;
      };
      
      //Singleton, don't call
      TransportDriverImpl();
      ~TransportDriverImpl();


      void thread();

      void addClient(Client* client);
      void removeClient(Client* client);

   private:

      // build the FD set to use in a select to find out when process bust be called again
      void buildFdSet(resip::FdSet& fdset);
      
      void process();
      bool processTransports();

      std::vector<Client*> mClients;

      resip::Mutex mMutex;
};

typedef Loki::SingletonHolder< TransportDriverImpl,                                                      
                               Loki::CreateUsingNew, 
                               Loki::DefaultLifetime, 
                               Loki::ClassLevelLockable>  TransportDriver;

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
