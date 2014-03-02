#ifndef TransportDriver_hxx
#define TransportDriver_hxx

#include "resip/stack/TimerQueue.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Mutex.hxx"

#include <sys/types.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include <memory>

//#ifndef WIN32
//#include <sys/select.h> // posix
//#endif

namespace resip
{
    class Transport;
}

class TransportDriver : public resip::ThreadIf
{
   public:
      class Client
      {
         public:
            virtual ~Client()=0;
            virtual void process(resip::FdSet& fdset) = 0;
            virtual void registerWithTransportDriver();
            virtual void unregisterFromTransportDriver();
            virtual void buildFdSet(resip::FdSet& fdset) = 0;
            virtual resip::Transport* getTransport() { return 0; }
      };
      
      void addClient(Client* client);
      void removeClient(Client* client);
      resip::Transport* getClientTransport(unsigned int transportKey);
      static TransportDriver& instance();

   protected:
      void thread();

   private:
      //Singleton, don't call
      friend class std::auto_ptr<TransportDriver>;      
      TransportDriver();
      ~TransportDriver();

      // build the FD set to use in a select to find out when process bust be called again
      void buildFdSet(resip::FdSet& fdset);
      
      void process();
      bool processTransports();

      unsigned int mNextTransportKey;
      std::vector<Client*> mClients;
      typedef std::map<unsigned int, resip::Transport*> TransportMap;
      TransportMap mTransports;
      resip::Mutex mMutex;

      //Singleton
      static std::auto_ptr<TransportDriver> mInstance;
      static resip::Mutex mInstanceMutex;      
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
