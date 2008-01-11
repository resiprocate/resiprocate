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
 * This file contains the defintion of the Netxx::Probe_impl class.
**/

#ifndef _Netxx_Probe_impl_h_
#define _Netxx_Probe_impl_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Probe.h>
#include <Netxx/Timeout.h>

// system includes
#include <vector>
#include <utility>

namespace Netxx {

/**
 * Probe_impl is the class that does the actuall probing of socket file
 * descriptors. The implementation is chosen at compile time, an example
 * would be Probe_select.cxx that calls select(2).
**/
class Probe_impl {
public:
    typedef std::vector< std::pair<socket_type, Probe::ready_type> > probe_type;

    Probe_impl (void);

    Probe_impl (const Probe_impl &other);

    Probe_impl& operator= (const Probe_impl &other);

    void swap (Probe_impl &other);

    ~Probe_impl (void);

    void add (socket_type socketfd, Probe::ready_type rt=Probe::ready_none);

    void remove (socket_type socketfd);

    void clear (void);

    probe_type probe (const Timeout &timeout, Probe::ready_type rt=Probe::ready_none);
private:
    void *pimpl_;
}; // end Netxx::Probe_impl class

} // end Netxx namespace
#endif
