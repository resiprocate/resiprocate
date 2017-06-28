
#include "rutil/Logger.hxx"
#include "QpidProtonThread.hxx"

#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/connection.hpp>
#include <proton/thread_safe.hpp>
#include <proton/tracker.hpp>
#include <proton/source_options.hpp>

using proton::sender_options;
using proton::source_options;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

QpidProtonThread::QpidProtonThread(const std::string &u)
   : mRetryDelay(2000),
     mPending(0),
     mUrl(u),
     mFifo(0, 0),
     mReadyToSend(*this),
     mReadyToShutdown(*this)
{
}

QpidProtonThread::~QpidProtonThread()
{
}

void
QpidProtonThread::on_container_start(proton::container &c)
{
   InfoLog(<<"QpidProtonThread::on_container_start invoked");
   proton::connection_options co;
   mSender = c.open_sender(mUrl, co);
}

void
QpidProtonThread::on_sender_open(proton::sender &)
{
   InfoLog(<<"sender ready for queue " << mUrl);
}

void
QpidProtonThread::on_sender_close(proton::sender &r)
{
   DebugLog(<<"sender closed");
}

void
QpidProtonThread::on_transport_error(proton::transport &t)
{
   WarningLog(<<"transport closed unexpectedly, will try to re-establish connection");
   StackLog(<<"sleeping for " << mRetryDelay << "ms before attempting to restart sender");
   sleepMs(mRetryDelay);
   t.connection().container().open_sender(mUrl);
}

void
QpidProtonThread::on_sendable(proton::sender& s)
{
   StackLog(<<"on_sendable invoked");
   doSend();
}

void
QpidProtonThread::on_tracker_accept(proton::tracker &t)
{
   StackLog(<<"on_tracker_accept: mPending = " << --mPending);
   if(isShutdown() && !mFifo.messageAvailable() && mPending == 0)
   {
      StackLog(<<"no more messages outstanding, shutting down");
      mSender.container().stop();
   }
}

void
QpidProtonThread::thread()
{
   while(!isShutdown())
   {
      try
      {
         StackLog(<<"trying to start Qpid Proton container");
         proton::default_container(*this).run();
      }
      catch(const std::exception& e)
      {
         ErrLog(<<"Qpid Proton container stopped by exception: " << e.what());
      }
      if(!isShutdown())
      {
         StackLog(<<"sleeping for " << mRetryDelay << "ms before attempting to restart container");
         sleepMs(mRetryDelay);
      }
   }
   DebugLog(<<"Qpid Proton thread finishing");
}

void
QpidProtonThread::sendMessage(const resip::Data& msg)
{
   mFifo.add(new Data(msg), TimeLimitFifo<Data>::InternalElement);
   proton::returned<proton::connection> ts_c = proton::make_thread_safe(mSender.connection());
   ts_c.get()->event_loop()->inject(mReadyToSend);
   StackLog(<<"QpidProtonThread::sendMessage added a message to the FIFO");
}

void
QpidProtonThread::doSend()
{
   if(!mSender.active())
   {
      StackLog(<<"doSend: mSender.active() == false, not trying to send");
      return;
   }
   while(mSender.credit() && mFifo.messageAvailable())
   {
      try
      {
         StackLog(<<"doSend trying to send a message");
         SharedPtr<Data> body(mFifo.getNext());
         proton::message msg;
         msg.body(body->c_str());
         mSender.send(msg);
         StackLog(<<"doSend: mPending = " << ++mPending);
      }
      catch(const std::exception& e)
      {
         ErrLog(<<"failed to send a message: " << e.what());
         return;
      }
   }
   if(mFifo.messageAvailable())
   {
      StackLog(<<"doSend still has messages to send, but no credit remaining");
   }
}

void
QpidProtonThread::shutdown()
{
   if(isShutdown())
   {
      DebugLog(<<"shutdown already in progress!");
      return;
   }
   DebugLog(<<"trying to shutdown the Qpid Proton container");
   ThreadIf::shutdown();
   if(!mFifo.messageAvailable() && mPending == 0)
   {
      StackLog(<<"no messages outstanding, shutting down immediately");
      proton::returned<proton::connection> ts_c = proton::make_thread_safe(mSender.connection());
      ts_c.get()->event_loop()->inject(mReadyToShutdown);
   }
   else
   {
      StackLog(<<"waiting to close connection, mFifo.size() = " << mFifo.size()
               << " and mPending = " << mPending);
   }
}

void
QpidProtonThread::ready_to_shutdown::operator()()
{
   StackLog(<<"ready_to_shutdown::operator(): closing sender");
   mThread.mSender.container().stop();
}


/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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
