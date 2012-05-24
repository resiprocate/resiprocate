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
 * This file contains the defintion of the Netxx::ServerBase class.
**/

#ifndef _Netxx_ServerBase_h_
#define _Netxx_ServerBase_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Timeout.h>
#include <Netxx/Probe.h>
#include <Netxx/ProbeInfo.h>
#include "Socket.h"
#include "Probe_impl.h"

// standard includes
#include <string>
#include <vector>
#include <map>

namespace Netxx {
    class Address;

/**
 * The Netxx::ServerBase class implements common server code for other Netxx
 * server classes.
**/
class ServerBase {
public:

    explicit ServerBase (const Timeout &timeout=Timeout());

    virtual ~ServerBase (void);

    void bind_to(const Address &addr, bool stream_server);

    void get_socket_list (Socket *&sockets, size_type &size);

    Socket* get_readable_socket (void);

    void set_timeout (const Timeout &timeout);

    const Timeout& get_timeout (void) const;

    bool has_socket (socket_type socketfd) const;

    const ProbeInfo* get_probe_info (void) const;
private:
    Timeout timeout_;
    Socket *sockets_;
    size_type sockets_size_;
    std::map<socket_type, Socket*> sockets_map_;
    std::vector<std::string> files_;

    ProbeInfo pi_;
    Probe probe_;

    ServerBase (const ServerBase&);
    ServerBase& operator= (const ServerBase&);
}; // end Netxx::ServerBase class

} // end Netxx namespace
#endif
