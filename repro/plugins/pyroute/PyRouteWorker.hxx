#ifndef PYROUTE_WORKER_HXX
#define PYROUTE_WORKER_HXX

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

#include "PyThreadSupport.hxx"

namespace repro
{

class PyRouteWork : public ProcessorMessage
{
   public:
      PyRouteWork(Processor& proc,
                  const resip::Data& tid,
                  resip::TransactionUser* passedtu,
                  resip::SipMessage& message);

      virtual ~PyRouteWork();

      resip::SipMessage& mMessage;
      int mResponseCode;
      resip::Data mResponseMessage;
      std::vector<resip::Data> mTargets;

      virtual PyRouteWork* clone() const;

      virtual EncodeStream& encode(EncodeStream& ostr) const;
      virtual EncodeStream& encodeBrief(EncodeStream& ostr) const;

      bool hasResponse() { return mResponseCode >= 0; };
};

class PyRouteWorker : public Worker
{
   public:
      PyRouteWorker(PyInterpreterState* interpreterState, Py::Callable& action);
      virtual ~PyRouteWorker();

      virtual PyRouteWorker* clone() const;

      virtual void onStart();
      virtual bool process(resip::ApplicationMessage* msg);

   protected:
      PyInterpreterState* mInterpreterState;
      PyExternalUser* mPyUser;
      Py::Callable& mAction;
};

}


#endif

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

