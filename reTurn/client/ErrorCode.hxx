#ifndef ERRORCODE_HXX
#define ERRORCODE_HXX

#include <asio/error_code.hpp>

namespace reTurn {

typedef int ErrorType;

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
static const ErrorType StrayResponse                         = ErrorBase + 16;
static const ErrorType UnknownRequiredAttributes             = ErrorBase + 17;
} 

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
