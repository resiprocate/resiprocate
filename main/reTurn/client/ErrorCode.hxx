#ifndef ERRORCODE_HXX
#define ERRORCODE_HXX

#include <asio/error_code.hpp>

namespace reTurn {

typedef asio::error_code::value_type ErrorType;

static const ErrorType Success                               = 0;
static const ErrorType GeneralError                          = -1;

static const ErrorType ErrorBase                             = 8000;

static const ErrorType MissingAuthenticationAttributes       = ErrorBase + 1;
static const ErrorType BufferTooSmall                        = ErrorBase + 2;
static const ErrorType BadMessageIntegrity                   = ErrorBase + 3;
static const ErrorType ErrorParsingMessage                   = ErrorBase + 4;
static const ErrorType NoAllocation                          = ErrorBase + 5;
static const ErrorType NoActiveDestination                   = ErrorBase + 6;
static const ErrorType ReadError                             = ErrorBase + 7;
static const ErrorType ResponseTimeout                       = ErrorBase + 8;
static const ErrorType FrameError                            = ErrorBase + 9;
static const ErrorType InvalidChannelNumberReceived          = ErrorBase + 10;
static const ErrorType MissingAttributes                     = ErrorBase + 11;
static const ErrorType UnknownRemoteAddress                  = ErrorBase + 12;
static const ErrorType InvalidRequestedTransport             = ErrorBase + 13;
static const ErrorType NotConnected                          = ErrorBase + 14;
static const ErrorType AlreadyAllocated                      = ErrorBase + 15;
} 

#endif


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
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

