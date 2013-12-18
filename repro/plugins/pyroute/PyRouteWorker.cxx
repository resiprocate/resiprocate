
#include <memory>

/* Using the PyCXX API for C++ Python integration
 * It is extremely convenient and avoids the need to write boilerplate
 * code for handling the Python reference counts.
 * It is licensed under BSD terms compatible with reSIProcate */
#include <Python.h>
#include <CXX/Objects.hxx>

#include "rutil/Logger.hxx"
#include "resip/stack/Helper.hxx"
#include "repro/Plugin.hxx"
#include "repro/Processor.hxx"
#include "repro/ProcessorMessage.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Worker.hxx"

#include "PyRouteWorker.hxx"
#include "PyThreadSupport.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace repro;

PyRouteWork::PyRouteWork(Processor& proc,
                  const resip::Data& tid,
                  resip::TransactionUser* passedtu,
                  resip::SipMessage& message)
    : ProcessorMessage(proc,tid,passedtu),
      mMessage(message)
{
}

PyRouteWork::~PyRouteWork()
{
}

PyRouteWork*
PyRouteWork::clone() const
{
   return new PyRouteWork(*this);
}

EncodeStream&
PyRouteWork::encode(EncodeStream& ostr) const
{
   ostr << "PyRouteWork(tid="<<mTid<<")";
   return ostr;
}

EncodeStream&
PyRouteWork::encodeBrief(EncodeStream& ostr) const
{
   return encode(ostr);
}

PyRouteWorker::PyRouteWorker(PyInterpreterState* interpreterState, Py::Callable& action)
    : mInterpreterState(interpreterState),
      mPyUser(0),
      mAction(action)
{
}

PyRouteWorker::~PyRouteWorker()
{
   if(mPyUser)
   {
      delete mPyUser;
   }
}

PyRouteWorker*
PyRouteWorker::clone() const
{
   PyRouteWorker* worker = new PyRouteWorker(*this);
   worker->mPyUser = 0;
   return worker;
}

void
PyRouteWorker::onStart()
{
   DebugLog(<< "creating new PyThreadState");
   mPyUser = new PyExternalUser(mInterpreterState);
}

bool
PyRouteWorker::process(resip::ApplicationMessage* msg)
{
   PyRouteWork* work = dynamic_cast<PyRouteWork*>(msg);
   if(!work)
   {
      WarningLog(<< "received unexpected message");
      return false;
   }

   DebugLog(<<"handling a message");

   resip::SipMessage& message = work->mMessage;
   // Get the Global Interpreter Lock
   StackLog(<< "getting lock...");
   assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);
   Py::String reqUri(message.header(resip::h_RequestLine).uri().toString().c_str());
   Py::Tuple args(1);
   args[0] = reqUri;
   Py::List routes;
   try
   {
      StackLog(<< "invoking mAction");
      routes = mAction.apply(args);
   }
   catch (const Py::Exception& ex)
   {
      WarningLog(<< "PyRoute mAction failed: " << Py::value(ex));
      WarningLog(<< Py::trace(ex));
      return false;
   }
   DebugLog(<< "got " << routes.size() << " result(s).");
   for(
      Py::Sequence::iterator i = routes.begin();
      i != routes.end();
      i++)
   {
      Py::String target(*i);
      resip::Data target_s(target.as_std_string());
      DebugLog(<< "processing result: " << target_s);
      work->mTargets.push_back(target_s);
   }
   
   return true;
}


/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock http://danielpocock.com  All rights reserved.
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

