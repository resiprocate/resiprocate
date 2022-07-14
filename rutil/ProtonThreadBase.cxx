
#include <chrono>
#include <thread>

#include "Logger.hxx"
#include "Subsystem.hxx"
#include "ProtonThreadBase.hxx"

#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/message.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/connection.hpp>
#include <proton/connection_options.hpp>
#include <proton/container.hpp>
#include <proton/reconnect_options.hpp>
#include <proton/tracker.hpp>
#include <proton/source_options.hpp>
#include <proton/work_queue.hpp>

// Cajun JSON
#include "cajun/json/reader.h"
#include "cajun/json/writer.h"
#include "cajun/json/elements.h"

using proton::receiver_options;
using proton::source_options;

#define RESIPROCATE_SUBSYSTEM Subsystem::QPIDPROTON

using namespace resip;
using namespace std;

ProtonThreadBase::ProtonReceiverBase::ProtonReceiverBase(const std::string &u,
   std::chrono::duration<long int> maximumAge,
   std::chrono::duration<long int> retryDelay)
   : mMaximumAge(maximumAge),
     mRetryDelay(retryDelay),
     mUrl(u),
     mWorkQueue(nullptr),
     mFifo(0, 0)
{
}

ProtonThreadBase::ProtonReceiverBase::~ProtonReceiverBase()
{
}

ProtonThreadBase::ProtonThreadBase(std::chrono::duration<long int> retryDelay)
   : mRetryDelay(retryDelay)
{
}

ProtonThreadBase::~ProtonThreadBase()
{
}

void
ProtonThreadBase::on_container_start(proton::container &c)
{
   for(auto rb : mReceivers)
   {
      proton::reconnect_options rec;
      rec.delay(proton::duration(std::chrono::milliseconds(rb->mRetryDelay).count()));
      proton::connection_options co;
      co.reconnect(rec);
      rb->mReceiver = c.open_receiver(rb->mUrl, co);
   }
}

void
ProtonThreadBase::on_connection_open(proton::connection& conn)
{
}

void
ProtonThreadBase::on_receiver_open(proton::receiver &r)
{
   for(auto rb : mReceivers)
   {
      if(rb->mReceiver == r)
      {
         InfoLog(<<"receiver ready for queue " << rb->mUrl);
         rb->mWorkQueue = &rb->mReceiver.work_queue();
         return;
      }
   }
   ErrLog(<<"unexpected receiver: " << r);
}

void
ProtonThreadBase::on_receiver_close(proton::receiver &r)
{
   InfoLog(<<"receiver closed");
}

void
ProtonThreadBase::on_transport_error(proton::transport &t)
{
   WarningLog(<<"transport closed unexpectedly, reason: " << t.error());
}

void
ProtonThreadBase::on_message(proton::delivery &d, proton::message &m)
{
   const proton::receiver& r = d.receiver();
   for(auto rb : mReceivers)
   {
      if(rb->mReceiver == r)
      {
         const proton::timestamp::numeric_type& ct = m.creation_time().milliseconds();
         StackLog(<<"message creation time (ms): " << ct);
         if(ct > 0 && rb->mMaximumAge > (std::chrono::duration<long int>::zero()))
         {
            const proton::timestamp::numeric_type threshold = ResipClock::getTimeMs() - std::chrono::milliseconds(rb->mMaximumAge).count();
            if(ct < threshold)
            {
               DebugLog(<<"dropping a message because it is too old: " << threshold - ct << "ms");
               return;
            }
         }
         // get body as std::stringstream
         std::string _json;
         try
         {
            _json = proton::get<std::string>(m.body());
         }
         catch(proton::conversion_error& ex)
         {
            ErrLog(<<"failed to extract message body as string: " << ex.what());
            return;
         }
         StackLog(<<"on_message received: " << _json);
         std::stringstream stream;
         stream << _json;

         // extract elements from JSON
         json::Object *elemRootFile = new json::Object();
         if(!elemRootFile)
         {
            ErrLog(<<"failed to allocate new json::Object()"); // FIXME
            return;
         }
         try
         {
            json::Reader::Read(*elemRootFile, stream);
         }
         catch(json::Reader::ScanException& ex)
         {
            ErrLog(<<"failed to scan JSON message: " << ex.what() << " message body: " << _json);
            return;
         }
         rb->mFifo.add(elemRootFile, TimeLimitFifo<json::Object>::InternalElement);

         return;
      }
   }
   ErrLog(<<"unexpected receiver: " << r);
}

void
ProtonThreadBase::thread()
{
   while(!isShutdown())
   {
      try
      {
         mContainer.reset(new proton::container(*this));
         mContainer->run();
      }
      catch(exception& e)
      {
         ErrLog(<<"ProtonThreadBase::thread container threw " << e.what());
      }
      if(!isShutdown())
      {
         WarningLog(<<"Proton container stopped unexpectedly, sleeping for " << std::chrono::milliseconds(mRetryDelay).count() << "ms before attempting to restart container");
         std::this_thread::sleep_for(mRetryDelay);
      }
   }
   mContainer.reset();
   InfoLog(<<"ProtonThreadBase::thread container stopped");
}

void
ProtonThreadBase::shutdown()
{
   if(isShutdown())
   {
      DebugLog(<<"shutdown already in progress!");
      return;
   }
   DebugLog(<<"trying to shutdown the Qpid Proton container");
   ThreadIf::shutdown();
   if(!mReceivers.empty())
   {
      if(mReceivers.front()->mWorkQueue)
      {
         mReceivers.front()->mWorkQueue->add(make_work(&ProtonThreadBase::doShutdown, this));
      }
      return;
   }
}

void
ProtonThreadBase::doShutdown()
{
   StackLog(<<"closing container");

   if(mContainer)
   {
      mContainer->stop();
   }
}

/* ====================================================================
 *
 * Copyright (C) 2022 Daniel Pocock https://danielpocock.com
 * Copyright (C) 2022 Software Freedom Institute SA https://softwarefreedom.institute
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */
