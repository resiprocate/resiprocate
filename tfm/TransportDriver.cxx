#include "resip/stack/Transport.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Mutex.hxx"
#include "tfm/TransportDriver.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

std::auto_ptr<TransportDriver> TransportDriver::mInstance;
resip::Mutex TransportDriver::mInstanceMutex;      


TransportDriver& TransportDriver::instance()
{
   if (mInstance.get())
   {
      return *mInstance.get();
   }
   else
   {
      Lock guard(mInstanceMutex);
      if (!mInstance.get())
      {
         mInstance = auto_ptr<TransportDriver>(new TransportDriver());
      }
      return *mInstance.get();
   }
}
   

TransportDriver::TransportDriver() : 
   mNextTransportKey(1)
{
   run();
}

TransportDriver::~TransportDriver()
{
   shutdown();
   join();
}

TransportDriver::Client::~Client()
{
}

void 
TransportDriver::Client::registerWithTransportDriver()
{
   TransportDriver::instance().addClient(this);
}

void 
TransportDriver::Client::unregisterFromTransportDriver()
{
   TransportDriver::instance().removeClient(this);
}

void 
TransportDriver::addClient(Client* client)
{
   Lock lock(mMutex);
   if (find(mClients.begin(), mClients.end(), client) == mClients.end())
   {
      unsigned int transportKey = mNextTransportKey++;
      Transport* transport = client->getTransport();
      if(transport)
      {
         transport->setKey(transportKey);
         mTransports[transportKey] = transport;
      }
      mClients.push_back(client);
   }
}

void 
TransportDriver::removeClient(Client* client)
{
   Lock lock(mMutex);
   Transport* transport = client->getTransport();
   if(transport)
   {
      mTransports.erase(transport->getKey());
   }
   mClients.erase(remove(mClients.begin(), mClients.end(), client), mClients.end());    
}

resip::Transport* 
TransportDriver::getClientTransport(unsigned int transportKey)
{
   Lock lock(mMutex);
   TransportMap::iterator it = mTransports.find(transportKey);
   if(it != mTransports.end())
   {
       return it->second;
   }
   return 0;
}

void
TransportDriver::thread()
{
   while (!isShutdown())
   {
      try
      {
         process();
      }
      catch(BaseException& e)
      {
         WarningLog (<< "Caught: " << e << " in transport driver");
      }
      catch(...)
      {
         WarningLog (<< "Caught unknown exception in transport driver");
      }
   }
}

void
TransportDriver::process()
{
   FdSet fdset;
   buildFdSet(fdset); 
   fdset.selectMilliSeconds(25); //5 ms granularity
   
   Lock lock(mMutex);
   for (vector<Client*>::iterator it =  mClients.begin();
        it != mClients.end(); it++)
   {
      (*it)->process(fdset);
   }
}

void
TransportDriver::buildFdSet(FdSet& fdset)
{
   Lock lock(mMutex);
   for (vector<Client*>::iterator it =  mClients.begin();
        it != mClients.end(); it++)
   {
      (*it)->buildFdSet(fdset);
   }
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
