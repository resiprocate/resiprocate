/*
 * Copyright (C) 2001-2003 Peter J Jones (pjones@pmade.org)
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** @file
 * This file contains the definitions of the Netxx resolve functions. These
 * functions resolve host names and service names based on a compile time
 * test to determin which system calls to use.
**/

#ifndef _Netxx_Resolve_h_
#define _Netxx_Resolve_h_

// Netxx includes
#include "Netxx/Types.h"
#include "Netxx/Peer.h"

// standard includes
#include <vector>

namespace Netxx {

    //####################################################################
    /** 
     * Resolve the given hostname and add each resulting address into a
     * given vector of Peer classes.
     *
     * @param hostname The hostname to resolve.
     * @param port The port used when creating Netxx::Peer classes.
     * @param use_ipv6 Accept IPv6 addresses.
     * @param addrs The vector to add peer addresses to.
     * @author Peter Jones
    **/
    //####################################################################
    void resolve_hostname (const char *hostname, port_type port, bool use_ipv6, std::vector<Peer> &addrs);

    //####################################################################
    /** 
     * Resolve the given service name and return the port number. This
     * function does not take the protocol type and therefor will use "tcp"
     * everytime. This is a simplification but may break on extreamly rare
     * situations.
     *
     * @param service The name of the server to resolve.
     * @return The port number for that service or 0 if there was an error.
     * @author Peter Jones
    **/
    //####################################################################
    port_type resolve_service (const char *service);

} // end Netxx namespace
#endif
