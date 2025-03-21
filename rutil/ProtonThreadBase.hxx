#ifndef PROTONTHREADBASE_HXX
#define PROTONTHREADBASE_HXX

#include <sstream>
#include <string>

#include "ThreadIf.hxx"
#include "TimeLimitFifo.hxx"

#include <proton/connection.hpp>
#include <proton/function.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/receiver.hpp>
#include <proton/sender.hpp>
#include <proton/transport.hpp>
#include <proton/work_queue.hpp>

#include "cajun/json/elements.h"

namespace resip {

class ProtonThreadBase : public resip::ThreadIf, proton::messaging_handler
{
public:

   class ProtonReceiverBase
   {
   public:
      ProtonReceiverBase(const std::string &u,
         std::chrono::duration<long int> maximumAge,
         std::chrono::duration<long int> retryDelay);
      virtual ~ProtonReceiverBase();
      std::chrono::duration<long int> mMaximumAge;
      std::chrono::duration<long int> mRetryDelay;
      std::string mUrl;
      proton::receiver mReceiver;
      proton::work_queue* mWorkQueue;
      resip::TimeLimitFifo<proton::message> mFifo;
      std::unique_ptr<json::Object> getBodyAsJSON(const proton::message &m);
   protected:
      resip::TimeLimitFifo<proton::message>& getFifo() { return mFifo; };
   };

   class ProtonSenderBase
   {
   public:
      ProtonSenderBase(const std::string &u,
         std::chrono::duration<long int> retryDelay = std::chrono::seconds(2));
      virtual ~ProtonSenderBase();
      std::chrono::duration<long int> mRetryDelay;
      std::string mUrl;
      proton::sender mSender;
      proton::work_queue* mWorkQueue;
      resip::TimeLimitFifo<Data> mFifo;
      uint64_t mPending;

      void sendMessage(const resip::Data& msg);
      void doSend();
   protected:
      resip::TimeLimitFifo<Data>& getFifo() { return mFifo; };
   };

   ProtonThreadBase(std::chrono::duration<long int> mRetryDelay = std::chrono::seconds(2));
   virtual ~ProtonThreadBase();

   void addReceiver(std::shared_ptr<ProtonReceiverBase> rx);
   void addSender(std::shared_ptr<ProtonSenderBase> tx);

   std::shared_ptr<ProtonSenderBase> getSenderForReply(const std::string& replyTo);

   void on_container_start(proton::container &c) override;
   void on_connection_open(proton::connection &conn) override;
   void on_receiver_open(proton::receiver &) override;
   void on_receiver_close(proton::receiver &) override;
   void on_sender_open(proton::sender &) override;
   void on_sender_close(proton::sender &) override;
   void on_transport_error(proton::transport &t) override;
   void on_message(proton::delivery &d, proton::message &m) override;
   void on_sendable(proton::sender &s) override;
   void on_tracker_accept(proton::tracker &t) override;

   virtual void thread() override;
   virtual void shutdown() override;


private:
   bool checkSenderShutdown();
   void doShutdown();

   void openReceiver(std::shared_ptr<ProtonReceiverBase> rx);
   void openSender(std::shared_ptr<ProtonSenderBase> tx);

   std::chrono::duration<long int> mRetryDelay;

   std::shared_ptr<proton::container> mContainer;
   proton::connection mConnection;
   bool mConnected;

   std::vector<std::shared_ptr<ProtonReceiverBase>> mReceivers;
   std::vector<std::shared_ptr<ProtonSenderBase>> mSenders;

   bool mStarted = false;
};

} // namespace

#endif

/* ====================================================================
 *
 * Copyright 2016-2022 Daniel Pocock https://danielpocock.com
 * Copyright 2022 Software Freedom Institute LLC https://softwarefreedom.institute
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
