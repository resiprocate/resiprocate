
#include <memory>

/* Using the PyCXX API for C++ Python integration
 * It is extremely convenient and avoids the need to write boilerplate
 * code for handling the Python reference counts.
 * It is licensed under BSD terms compatible with reSIProcate */
#include <Python.h>
#include <CXX/Objects.hxx>

#include "rutil/Logger.hxx"
#include "resip/stack/Cookie.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/HeaderFieldValueList.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/UnknownHeaderType.hxx"
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
      mMessage(message),
      mResponseCode(-1)
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
   resip_assert(mPyUser);
   PyExternalUser::Use use(*mPyUser);

   // arg 1: SIP method
   Py::String reqMethod(getMethodName(message.header(resip::h_RequestLine).method()).c_str());

   // arg 2: request URI
   Py::String reqUri(message.header(resip::h_RequestLine).uri().toString().c_str());

   // arg 3: a subset of the SIP headers
   Py::Dict headers;
   headers["From"] = Py::String(message.header(resip::h_From).uri().toString().c_str());
   headers["To"] = Py::String(message.header(resip::h_To).uri().toString().c_str());
   if(message.exists(resip::h_ContentType))
   {
      const resip::HeaderFieldValue hfv = message.header(resip::h_ContentType).getHeaderField();
      headers["Content-Type"] = Py::String(hfv.getBuffer(), hfv.getLength(), "utf8");
   }
   const resip::SipMessage::UnknownHeaders& unknowns = message.getRawUnknownHeaders();
   resip::SipMessage::UnknownHeaders::const_iterator it = unknowns.begin();
   for( ; it != unknowns.end(); it++)
   {
      const resip::Data& name = it->first;
      StackLog(<<"found unknown header: " << name);
      resip::HeaderFieldValueList* hfvl = it->second;
      if(!hfvl->empty())
      {
         resip::HeaderFieldValue* hfv = hfvl->front();
         headers[name.c_str()] = Py::String(hfv->getBuffer(), hfv->getLength(), "utf8");
      }
      if(hfvl->size() > 1)
      {
         // TODO - if multiple values exist, put them in a Py::List
         WarningLog(<<"ignoring additional values for header " << name);
      }
   }

   // arg 4: transport type
   Py::String transportType(
      getTransportNameFromType(message.getReceivedTransportTuple().getType()));

   // arg 5: body
   Py::String body("");
   const resip::HeaderFieldValue& bodyHfv = message.getRawBody();
   if(bodyHfv.getLength() > 0)
   {
      // FIXME: do we always need to copy the whole body?
      // could we give Python a read-only pointer to the body data?
      body = Py::String(bodyHfv.getBuffer(), bodyHfv.getLength(), "utf8");
   }

   // arg 6: cookies (if the message was received over a WebSocket transport)
   const resip::CookieList& _cookies = message.getWsCookies();
   Py::Dict cookies;
   for(
      resip::CookieList::const_iterator it = _cookies.begin();
      it != _cookies.end();
      it++)
   {
      ErrLog(<<"adding cookie: " << it->name());
      cookies[Py::String(it->name().c_str())] = Py::String(it->value().c_str());
   }

   // arg 7: For new headers that come back from the script
   Py::Dict newHeaders;

   Py::Tuple args(7);
   args[0] = reqMethod;
   args[1] = reqUri;
   args[2] = headers;
   args[3] = transportType;
   args[4] = body;
   args[5] = cookies;
   args[6] = newHeaders;
   Py::Object response;
   try
   {
      StackLog(<< "invoking mAction");
      response = mAction.apply(args);
   }
   catch (const Py::Exception& ex)
   {
      WarningLog(<< "PyRoute mAction failed: " << Py::value(ex));
      WarningLog(<< Py::trace(ex));
      work->mResponseCode = 500;
      return true;
   }

   // Did the script return a single numeric value?
   // If so, it is a SIP response code, the default SIP error string
   // will be selected by the stack
   if(response.isNumeric())
   {
      Py::Int responseCode(response);
      work->mResponseCode = responseCode;
      work->mResponseMessage = "";
      return true;
   }

   // Did the script return a tuple?
   // If so, it is a SIP response code and optionally a response string
   // to be used in the SIP response message
   // If no response string is present in the tuple, the default SIP
   // error string will be selected by the stack
   if(response.isTuple())
   {
      // Error response
      Py::Tuple err(response);
      if(err.size() < 1)
      {
         ErrLog(<<"Incomplete response object from PyRoute script");
         work->mResponseCode = 500;
         return true;
      }
      if(err.size() > 2)
      {
         WarningLog(<<"Excessive values in response from PyRoute script");
      }
      if(!err[0].isNumeric())
      {
         ErrLog(<<"First value in response tuple must be numeric");
         work->mResponseCode = 500;
         return true;
      }
      Py::Int responseCode(err[0]);
      Py::String responseMessage;
      if(err.size() > 1)
      {
         if(!err[1].isString())
         {
            ErrLog(<<"Second value in response tuple must be a string, ignoring it");
         }
         responseMessage = err[1];
      }
      work->mResponseCode = responseCode;
      work->mResponseMessage = resip::Data(responseMessage.as_std_string());
      return true;
   }

   // If we get this far, the response should be a list of target URIs
   if(!response.isList())
   {
      ErrLog(<<"Unexpected response object from PyRoute script");
      work->mResponseCode = 500;
      return true;
   }

   Py::List routes(response);
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

   int newHeaderCount = newHeaders.size();
   DebugLog(<<"got " << newHeaderCount << " new/changed header(s)");
   for(Py::Dict::iterator it = newHeaders.begin();
      it != newHeaders.end();
      it++)
   {
      const Py::Dict::value_type vt(*it);
      resip::Data headerName(vt.first.str());
      resip::Data value(vt.second.str());
      DebugLog(<<"processing a header: " << headerName << ": " << value);
      resip::Headers::Type hType = resip::Headers::getType(headerName.data(), (int)headerName.size());
      if(hType == resip::Headers::UNKNOWN)
      {
         resip::ExtensionHeader h_Tmp(headerName.c_str());
         resip::ParserContainer<resip::StringCategory>& pc = message.header(h_Tmp);
         while(pc.begin() != pc.end())
         {
            pc.erase(pc.begin());
         }
         resip::StringCategory sc(value);
         pc.push_back(sc);
      }
      else
      {
         WarningLog(<<"Discarding header '"<<headerName<<"' from pyroute script, only extension headers permitted");
      }
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

