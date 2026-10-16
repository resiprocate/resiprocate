#if !defined(RESIP_SECURITY_TYPES_HXX)
#define RESIP_SECURITY_TYPES_HXX

namespace resip 
{ 

namespace SecurityTypes 
{ 
typedef enum
{
   OpenSSL
} SSLVendor;

/**
 * Note:
 * Before adding to the SSLType enum, please see the comments
 * about mTlsCtx in resip/stack/ssl/Security.hxx
 *
 * Security allows two modes of operation: dynamic version negotiation (also called SSLv23) 
 * and pinned to TLSv1 (1.0 only). It tracks a SSL_CTX for each of these 2 modes.  This allows
 * TLS transports in resip to use different TLS versions, however it is also difficult to 
 * expand this mechanism to support more versions of TLS without breaking ABI compatibility.
 * 
 * Instead resip offers a single configuration for controlling OpenSSL options that can be
 * used to disable TLS versions. This is the preferred mechanism for controlling TLS versions.
 * See Security::OpenSSLCTXSetOptions and Security::OpenSSLCTXClearOptions.
 *
 * Caveat:  using Security::OpenSSLCTXSetOptions is application wide and cannot be applied to
 * a single TLS transport.  We currently do NOT support different TLS versions for different
 * transports, other than the base existing TLSv1 option.
 */
typedef enum 
{ 
   NoSSL = 0,
   DynamicTLS = 1, // dynamic version negotiation - negotiates the highest version supported by both sides
   SSLv23 = 1,     // left in place for legacy reasons - same as Dynamic despite naming
   TLSv1 = 2       // pinned to TLSv1 (1.0 only)
} SSLType;

typedef enum
{
   None,
   Optional,
   Mandatory
} TlsClientVerificationMode;

}; 

};

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026, SIP Spectrum, Inc. http://sipspectrum.com
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
