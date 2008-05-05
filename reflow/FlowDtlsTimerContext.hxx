#if !defined(FlowDtlsTimerContext_hxx)
#define FlowDtlsTimerContext_hxx 

#include <rutil/SharedPtr.hxx>
#include <asio.hpp>
#include "dtls_wrapper/DtlsTimer.hxx"

/**
  This class is used to provide timer logic to the dtls
  wrapper code, so that DTLS messages can be retransmitted.

  Author: Scott Godin (sgodin AT SipSpectrum DOT com)
*/
namespace flowmanager
{

class FlowDtlsTimerContext: public dtls::DtlsTimerContext
{
  public:
     FlowDtlsTimerContext(asio::io_service& ioService);
     void addTimer(dtls::DtlsTimer *timer, unsigned int durationMs);
     void handleTimeout(dtls::DtlsTimer *timer, const asio::error_code& errorCode);

   private:
     asio::io_service& mIOService;
     std::map<dtls::DtlsTimer*, resip::SharedPtr<asio::deadline_timer> > mDeadlineTimers;  
};

}

#endif


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
