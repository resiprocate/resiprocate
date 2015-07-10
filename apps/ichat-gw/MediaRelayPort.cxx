#include "rutil/ResipAssert.h"

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <resip/stack/Symbols.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/Tuple.hxx>
//#include <rutil/DnsUtil.hxx>
//#include <rutil/ParseBuffer.hxx>
#include <resip/stack/Transport.hxx>

#include "AppSubsystem.hxx"
#include "MediaRelayPort.hxx"
#include <rutil/WinLeakCheck.hxx>

using namespace gateway;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

MediaRelayPort::MediaRelayPort() : mV4Fd(INVALID_SOCKET), mV6Fd(INVALID_SOCKET) 
{
}

MediaRelayPort::MediaRelayPort(resip::Socket& v4fd, resip::Tuple& v4tuple, resip::Socket& v6fd, resip::Tuple& v6tuple) : 
      mV4Fd(v4fd), mLocalV4Tuple(v4tuple), 
      mV6Fd(v6fd), mLocalV6Tuple(v6tuple)
{
}

MediaRelayPort::MediaRelayPort(resip::Socket& v4fd, resip::Tuple& v4tuple) : 
      mV4Fd(v4fd), mLocalV4Tuple(v4tuple), 
      mV6Fd(INVALID_SOCKET)
{
}

MediaRelayPort::~MediaRelayPort() 
{
#if defined(WIN32)
   if(mV4Fd != INVALID_SOCKET) closesocket(mV4Fd);
   if(mV6Fd != INVALID_SOCKET) closesocket(mV6Fd);
#else
   if(mV4Fd != INVALID_SOCKET) close(mV4Fd); 
   if(mV6Fd != INVALID_SOCKET) close(mV6Fd); 
#endif
}


/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

