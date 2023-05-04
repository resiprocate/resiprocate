
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

ProtonThreadBase::ProtonSenderBase::ProtonSenderBase(const std::string &u,
   std::chrono::duration<long int> retryDelay)
   : mRetryDelay(retryDelay),
     mUrl(u),
     mWorkQueue(nullptr),
     mFifo(0, 0),
     mPending(0)
{
   StackLog(<<"ProtonSenderBase::ProtonSenderBase : " << mUrl);
}

ProtonThreadBase::ProtonSenderBase::~ProtonSenderBase()
{
}

void
ProtonThreadBase::ProtonSenderBase::sendMessage(const resip::Data& msg)
{
   if(mFifo.add(new Data(msg), TimeLimitFifo<Data>::IgnoreTimeDepth))
   {
      StackLog(<<"QpidProtonThread::sendMessage added a message to the FIFO");
   }
   else
   {
      ErrLog(<<"mFifo rejected the message");
   }
   if(mWorkQueue)
   {
      mWorkQueue->add(proton::internal::v11::make_work(&ProtonThreadBase::ProtonSenderBase::doSend, this));
   }
}

void
ProtonThreadBase::ProtonSenderBase::doSend()
{
   try {
      StackLog(<<"checking for a message");
      while(mFifo.messageAvailable() && mSender.credit() > 0)
      {
         StackLog(<<"doSend trying to send a message");
         std::shared_ptr<Data> body(mFifo.getNext());
         proton::message msg;
         msg.body(body->c_str());
         mSender.send(msg);
         StackLog(<<"doSend: mPending = " << ++mPending);
      }
      if(mFifo.messageAvailable())
      {
         StackLog(<<"tick still has messages to send, but no credit remaining");
      }
   }
   catch(const std::exception& e)
   {
      ErrLog(<<"failed to send a message: " << e.what());
      return;
   }
}

ProtonThreadBase::ProtonThreadBase(std::chrono::duration<long int> retryDelay)
   : mRetryDelay(retryDelay),
     mConnected(false)
{
}

ProtonThreadBase::~ProtonThreadBase()
{
}

void
ProtonThreadBase::openReceiver(std::shared_ptr<ProtonReceiverBase> rx)
{
   StackLog(<<"calling open_receiver");
   if(mConnected)
   {
      rx->mReceiver = mConnection.open_receiver(rx->mUrl);
      rx->mWorkQueue = &rx->mReceiver.work_queue();
   }
   else
   {
      proton::reconnect_options rec;
      rec.delay(proton::duration(std::chrono::milliseconds(rx->mRetryDelay).count()));
      proton::connection_options co;
      co.reconnect(rec);
      rx->mReceiver = mContainer->open_receiver(rx->mUrl, co);
   }
}

void
ProtonThreadBase::openSender(std::shared_ptr<ProtonSenderBase> tx)
{
   StackLog(<<"calling open_sender for " << tx->mUrl);
   if(mConnected)
   {
      tx->mSender = mConnection.open_sender(tx->mUrl);
      tx->mWorkQueue = &tx->mSender.work_queue();
      tx->mWorkQueue->add(proton::internal::v11::make_work(&ProtonThreadBase::ProtonSenderBase::doSend, tx));
   }
   else
   {
      proton::reconnect_options rec;
      rec.delay(proton::duration(std::chrono::milliseconds(tx->mRetryDelay).count()));
      proton::connection_options co;
      co.reconnect(rec);
      tx->mSender = mContainer->open_sender(tx->mUrl, co);
   }
}

void
ProtonThreadBase::addReceiver(std::shared_ptr<ProtonReceiverBase> rx)
{
   mReceivers.push_back(rx);
   if(mStarted)
   {
      openReceiver(rx);
   }
}

void
ProtonThreadBase::addSender(std::shared_ptr<ProtonSenderBase> tx)
{
   mSenders.push_back(tx);
   if(mStarted)
   {
      openSender(tx);
   }
}

std::shared_ptr<ProtonThreadBase::ProtonSenderBase>
ProtonThreadBase::getSenderForReply(const std::string& replyTo)
{
   std::shared_ptr<ProtonSenderBase> sender = std::make_shared<ProtonThreadBase::ProtonSenderBase>(replyTo);
   addSender(sender);
   return sender;
}

void
ProtonThreadBase::on_container_start(proton::container &c)
{
   mStarted = true;
   for(auto rb : mReceivers)
   {
      openReceiver(rb);
   }
   for(auto sb : mSenders)
   {
      openSender(sb);
   }
}

void
ProtonThreadBase::on_connection_open(proton::connection& conn)
{
   InfoLog(<<"ProtonThreadBase::on_connection_open");
   mConnection = conn;
   mConnected = true;
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
ProtonThreadBase::on_sender_open(proton::sender &s)
{
   for(auto sb : mSenders)
   {
      if(sb->mSender == s)
      {
         InfoLog(<<"sender ready for queue " << sb->mUrl);
         sb->mWorkQueue = &s.work_queue();
         sb->doSend();
         return;
      }
   }
   ErrLog(<<"unexpected sender: " << s);
}

void
ProtonThreadBase::on_sender_close(proton::sender &r)
{
   DebugLog(<<"sender closed");
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
         rb->mFifo.add(new proton::message(m), TimeLimitFifo<proton::message>::InternalElement);

         return;
      }
   }
   ErrLog(<<"unexpected receiver: " << r);
}

std::unique_ptr<json::Object>
ProtonThreadBase::ProtonReceiverBase::getBodyAsJSON(const proton::message &m)
{
   std::unique_ptr<json::Object> result;
   // get body as std::stringstream
   std::string _json;
   try
   {
      _json = proton::get<std::string>(m.body());
   }
   catch(proton::conversion_error& ex)
   {
      ErrLog(<<"failed to extract message body as string: " << ex.what());
      return result;
   }
   StackLog(<<"on_message received: " << _json);
   std::stringstream stream;
   stream << _json;

   // extract elements from JSON
   json::Object* __json = new json::Object();
   if(!__json)
   {
      ErrLog(<<"failed to allocate new json::Object()");
      return result;
   }
   result.reset(__json);
   try
   {
      json::Reader::Read(*result, stream);
   }
   catch(json::Reader::ScanException& ex)
   {
      ErrLog(<<"failed to scan JSON message: " << ex.what() << " message body: " << _json);
      result.reset();
      return result;
   }
   return result;
}

void
ProtonThreadBase::on_sendable(proton::sender& s)
{
   StackLog(<<"on_sendable invoked");
   for(auto sb : mSenders)
   {
      if(sb->mSender == s)
      {
         sb->doSend();
      }
   }
}

bool
ProtonThreadBase::checkSenderShutdown()
{
   bool nothingOutstanding = true;
   for(auto sb : mSenders)
   {
      if(sb->mFifo.messageAvailable() || sb->mPending > 0)
      {
         nothingOutstanding = false;
      }
   }

   if(isShutdown() && nothingOutstanding)
   {
      StackLog(<<"no more messages outstanding, shutting down");
      return true;
   }
   return false;
}

void
ProtonThreadBase::on_tracker_accept(proton::tracker &t)
{
   for(auto sb : mSenders)
   {
      if(sb->mSender == t.sender())
      {
         StackLog(<<"on_tracker_accept: mPending = " << --sb->mPending);
      }
   }

   if(checkSenderShutdown())
   {
      StackLog(<<"no more messages outstanding, shutting down");
      doShutdown();
   }
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
   if(!checkSenderShutdown())
   {
      return;
   }
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
