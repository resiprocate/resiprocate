#include "repro/Dispatcher.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/WinLeakCheck.hxx"


namespace repro
{

Dispatcher::Dispatcher(std::auto_ptr<Worker> prototype,
                        resip::SipStack* stack,
                        int workers, 
                        bool startImmediately):
   mStack(stack),
   mFifo(0,0),
   mAcceptingWork(false),
   mShutdown(false),
   mStarted(false),
   mWorkerPrototype(prototype.release())
{
   for(int i=0; i<workers;i++)
   {
      mWorkerThreads.push_back(new WorkerThread(mWorkerPrototype->clone(),mFifo,mStack));
   }
   
   if(startImmediately)
   {
      startAll();
   }
}

Dispatcher::~Dispatcher()
{
   shutdownAll();
   
   std::vector<WorkerThread*>::iterator i;
   for(i=mWorkerThreads.begin(); i!=mWorkerThreads.end(); ++i)
   {
      delete *i;
   }

   mWorkerThreads.clear();

   while(!mFifo.empty())
   {
      delete mFifo.getNext();
   }
   
   delete mWorkerPrototype;
   
}

bool
Dispatcher::post(std::auto_ptr<resip::ApplicationMessage>& work)
{
   resip::ReadLock r(mMutex);
   if(mAcceptingWork)
   {
      mFifo.add(work.release(),
                  resip::TimeLimitFifo<resip::ApplicationMessage>::InternalElement);
      return true;
   }
   
   return false;
   
   //If we aren't accepting work, the auto ptr is not released. (We don't
   // take ownership, and the caller gets to handle the contents of the 
   // auto_ptr)
}

size_t
Dispatcher::fifoCountDepth() const 
{
   return mFifo.getCountDepth();
}

time_t
Dispatcher::fifoTimeDepth() const 
{
   return mFifo.getTimeDepth();
}

int
Dispatcher::workPoolSize() const 
{
   return (int)mWorkerThreads.size();
}

void
Dispatcher::stop()
{
   resip::WriteLock w(mMutex);
   mAcceptingWork=false;
}

void
Dispatcher::resume()
{
   resip::WriteLock w(mMutex);
   mAcceptingWork = !mShutdown;
}

void
Dispatcher::shutdownAll()
{
   resip::WriteLock w(mMutex);
   if(!mShutdown)
   {
      mAcceptingWork=false;
      mShutdown=true;
      
      std::vector<WorkerThread*>::iterator i;
      for(i=mWorkerThreads.begin(); i!=mWorkerThreads.end(); ++i)
      {
         (*i)->shutdown();
         (*i)->join();
      }
   }
}

void 
Dispatcher::startAll()
{
   resip::WriteLock w(mMutex);
   if(!mShutdown && !mStarted)
   {
      std::vector<WorkerThread*>::iterator i;
      for(i=mWorkerThreads.begin(); i!=mWorkerThreads.end(); ++i)
      {
         (*i)->run();
      }
      mStarted=true;
      mAcceptingWork=true;
   }
}

} //namespace repro

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
