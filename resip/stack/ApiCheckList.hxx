#if !defined(RESIP_APICHECKLIST_HXX)
#define RESIP_APICHECKLIST_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "resip/stack/ApiCheck.hxx"

// These types were chosen because they represent the exported stuff.


#include "rutil/GenericIPAddress.hxx"
#include "resip/stack/Connection.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/UdpTransport.hxx"

// Make an entry in the table
#define RESIP_TENT(x,y) { #x, sizeof(resip::x), y }

static ::resip::ApiCheck::ApiEntry anonymous_resipApiSizeList[] =
{
    // KEEP SORTED ALPHABETICALLY
    RESIP_TENT(Connection,"NEW_MSG_HEADER_SCANNER"),
    RESIP_TENT(Data,"RESIP_DATA_LOCAL_SIZE (configure)"),
    RESIP_TENT(DnsResult,"USE_IPV6"),
    RESIP_TENT(Headers,"PARTIAL_TEMPLATE_SPECIALIZATION"),
    RESIP_TENT(MsgHeaderScanner,"NEW_MSG_HEADER_SCANNER"),
    RESIP_TENT(SipMessage, "PARTIAL_TEMPLATE_SPECIALIZATION"),
    RESIP_TENT(TransportSelector,"USE_IPV6"),
    RESIP_TENT(Tuple,"USE_IPV6"),
    RESIP_TENT(UdpTransport,"NEW_MSG_HEADER_SCANNER"),
    RESIP_TENT(GenericIPAddress,"USE_IPV6")
};

#undef RESIP_TENT

static resip::ApiCheck
 anonymous_resipApiCheckObj(anonymous_resipApiSizeList,
                            sizeof(anonymous_resipApiSizeList)/sizeof(*anonymous_resipApiSizeList));

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
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
