#ifndef COMMANDTHREAD_HXX
#define COMMANDTHREAD_HXX

#include <sstream>
#include <string>

#include "rutil/ThreadIf.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/dum/DialogUsageManager.hxx"

#include <proton/function.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/receiver.hpp>
#include <proton/transport.hpp>
#include <proton/work_queue.hpp>

#include "cajun/json/elements.h"

#include "UserRegistrationClient.hxx"

namespace registrationagent {

class CommandThread : public resip::ThreadIf, proton::messaging_handler
{
public:
   CommandThread(const std::string &u);
   ~CommandThread();

   void on_container_start(proton::container &c);
   void on_connection_open(proton::connection &conn);
   void on_receiver_open(proton::receiver &);
   void on_receiver_close(proton::receiver &);
   void on_transport_error(proton::transport &t);
   void on_message(proton::delivery &d, proton::message &m);

   virtual void thread();
   virtual void shutdown();

   void processQueue(UserRegistrationClient& userRegistrationClient);

private:
   UInt64 mMaximumAge;
   unsigned int mRetryDelay;
   std::string mUrl;
   proton::receiver mReceiver;
   proton::work_queue* mWorkQueue;
   resip::TimeLimitFifo<json::Object> mFifo;

   void doShutdown();
};

} // namespace

#endif

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
