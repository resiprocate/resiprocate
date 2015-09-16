#include "tfm/tfdum/TfdTcpTransport.hxx"
#include "tfm/tfdum/TfdTestSipEndPoint.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

TfdTestSipEndPoint::TfdTestSipEndPoint(const resip::Uri& from,
                                       const resip::Uri& contact, 
                                       const resip::Uri& outboundProxy,
                                       const resip::Data& interfaceObj,
                                       resip::Security* security)
   : TestSipEndPoint(from, contact, outboundProxy, false, interfaceObj, security)
{
   setTransport(new TfdTcpTransport(mIncoming, mContact.uri().port(), V4, interfaceObj));
}
      
TfdTestSipEndPoint::TfdTestSipEndPoint(const resip::Uri& contact, 
                                       const resip::Uri& outboundProxy,
                                       const resip::Data& interfaceObj,
                                       resip::Security* security)
   : TestSipEndPoint(contact, outboundProxy, false, interfaceObj, security)
{
   setTransport(new TfdTcpTransport(mIncoming, mContact.uri().port(), V4, interfaceObj));
}
 

TfdTestSipEndPoint::~TfdTestSipEndPoint()
{
}

void 
TfdTestSipEndPoint::removeConnections()
{
   DebugLog(<< "TfdTestSipEndPoint::removeConnections");
   resip_assert(dynamic_cast<TfdTcpTransport*>(mTransport));
   dynamic_cast<TfdTcpTransport*>(mTransport)->removeConnections();
}      

TfdTestSipEndPoint::TerminateConnection::TerminateConnection(TfdTestSipEndPoint* from)
   : mEndPoint(*from)
{
}

void 
TfdTestSipEndPoint::TerminateConnection::operator()() 
{ 
   go(); 
}

void 
TfdTestSipEndPoint::TerminateConnection::operator()(boost::shared_ptr<Event> event)
{
   go();
}

void
TfdTestSipEndPoint::TerminateConnection::go()
{
   mEndPoint.removeConnections();
}

resip::Data
TfdTestSipEndPoint::TerminateConnection::toString() const
{
   return mEndPoint.getName() + ".terminateConnection()";
}

TfdTestSipEndPoint::Action* 
TfdTestSipEndPoint::terminateConnection()
{
   return new TerminateConnection(this);
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
