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
 * This file contains the implementation of the Netxx::Probe_impl class for
 * the select(2) system call.
**/

// select sucks
#define FD_SETSIZE 65536

// common header
#include "common.h"

// Netxx includes
#include "Probe_impl.h"
#include "Netxx/Types.h"
#include "Netxx/Timeout.h"
#include "OSError.h"

// system includes
#if defined(WIN32)
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/time.h>
# include <unistd.h>
# include <errno.h>
#endif

// standard includes
#include <memory>
#include <algorithm>
#include <string>
#include <list>
#include <set>
#include <cstring>

//####################################################################
namespace {
    struct pimpl {
	std::list<Netxx::socket_type> rd_sockets_;
	std::list<Netxx::socket_type> wr_sockets_;
	std::list<Netxx::socket_type> er_sockets_;
    };
}
//####################################################################
Netxx::Probe_impl::Probe_impl (void) {
    pimpl_ = new pimpl;
}
//####################################################################
Netxx::Probe_impl::Probe_impl (const Probe_impl &other) {
    std::auto_ptr<pimpl> ap(new pimpl); pimpl_ = ap.get();

    static_cast<pimpl*>(pimpl_)->rd_sockets_ = static_cast<pimpl*>(other.pimpl_)->rd_sockets_;
    static_cast<pimpl*>(pimpl_)->wr_sockets_ = static_cast<pimpl*>(other.pimpl_)->wr_sockets_;
    static_cast<pimpl*>(pimpl_)->er_sockets_ = static_cast<pimpl*>(other.pimpl_)->er_sockets_;

    ap.release();
}
//####################################################################
Netxx::Probe_impl& Netxx::Probe_impl::operator= (const Probe_impl &other) {
    Probe_impl tmp(other); swap(tmp);
    return *this;
}
//####################################################################
void Netxx::Probe_impl::swap (Probe_impl &other) {
    static_cast<pimpl*>(pimpl_)->rd_sockets_.swap(static_cast<pimpl*>(other.pimpl_)->rd_sockets_);
    static_cast<pimpl*>(pimpl_)->wr_sockets_.swap(static_cast<pimpl*>(other.pimpl_)->wr_sockets_);
    static_cast<pimpl*>(pimpl_)->er_sockets_.swap(static_cast<pimpl*>(other.pimpl_)->er_sockets_);
}
//####################################################################
Netxx::Probe_impl::~Probe_impl (void) {
    delete static_cast<pimpl*>(pimpl_);
}
//####################################################################
void Netxx::Probe_impl::add (socket_type socketfd, Probe::ready_type rt) {
    if (rt == Probe::ready_none || rt & Probe::ready_read)
	static_cast<pimpl*>(pimpl_)->rd_sockets_.push_back(socketfd);

    if (rt == Probe::ready_none || rt & Probe::ready_write)
	static_cast<pimpl*>(pimpl_)->wr_sockets_.push_back(socketfd);

    if (rt == Probe::ready_none || rt & Probe::ready_oobd)
	static_cast<pimpl*>(pimpl_)->er_sockets_.push_back(socketfd);
}
//####################################################################
void Netxx::Probe_impl::remove (socket_type socketfd) {
    static_cast<pimpl*>(pimpl_)->rd_sockets_.remove(socketfd);
    static_cast<pimpl*>(pimpl_)->wr_sockets_.remove(socketfd);
    static_cast<pimpl*>(pimpl_)->er_sockets_.remove(socketfd);
}
//####################################################################
void Netxx::Probe_impl::clear (void) {
    static_cast<pimpl*>(pimpl_)->rd_sockets_.clear();
    static_cast<pimpl*>(pimpl_)->wr_sockets_.clear();
    static_cast<pimpl*>(pimpl_)->er_sockets_.clear();
}
//####################################################################
Netxx::Probe_impl::probe_type Netxx::Probe_impl::probe (const Timeout &timeout, Probe::ready_type rt) {
    pimpl *p=static_cast<pimpl*>(pimpl_);
    probe_type return_value;

    fd_set rd_fdset, wr_fdset, er_fdset;
    fd_set *rd_fdptr, *wr_fdptr, *er_fdptr;
    timeval tmp_timeout;
    timeval *timeout_ptr;
    socket_type max_fd;
    int rc;

    for (;;) {
	if (timeout) {
	    tmp_timeout.tv_sec = timeout.get_sec();
	    tmp_timeout.tv_usec = timeout.get_usec();
	    timeout_ptr = &tmp_timeout;
	} else {
	    timeout_ptr = 0;
	}

	FD_ZERO(&rd_fdset);
	FD_ZERO(&wr_fdset);
	FD_ZERO(&er_fdset);

	std::set<socket_type> all_sockets;
	std::list<socket_type>::const_iterator it, end;

	if (rt == Probe::ready_none || rt & Probe::ready_read) {
	    for (it=p->rd_sockets_.begin(), end=p->rd_sockets_.end(); it!=end; ++it) {
		FD_SET(*it, &rd_fdset);
		all_sockets.insert(*it);
	    }
	    
	    rd_fdptr = &rd_fdset;
	} else {
	    rd_fdptr = 0;
	}

	if (rt == Probe::ready_none || rt & Probe::ready_write) {
	    for (it=p->wr_sockets_.begin(), end=p->wr_sockets_.end(); it!=end; ++it) {
		FD_SET(*it, &wr_fdset);
		all_sockets.insert(*it);
	    }

	    wr_fdptr = &wr_fdset;
	} else {
	    wr_fdptr = 0;
	}

	if (rt == Probe::ready_none || rt & Probe::ready_oobd) {
	    for (it=p->er_sockets_.begin(), end=p->er_sockets_.end(); it!=end; ++it) {
		FD_SET(*it, &er_fdset);
		all_sockets.insert(*it);
	    }

	    er_fdptr = &er_fdset;
	} else {
	    er_fdptr = 0;
	}

	if (all_sockets.empty()) {
	    return return_value;
	}

	std::set<socket_type>::const_iterator maxfd_it = std::max_element(all_sockets.begin(), all_sockets.end());
	max_fd = maxfd_it == all_sockets.end() ? 0 : *maxfd_it;

	if (max_fd > FD_SETSIZE) {
	    throw Exception("Netxx::Probe: too many sockets for select");
	}

	if ( (rc = select(max_fd+1, rd_fdptr, wr_fdptr, er_fdptr, timeout_ptr)) > 0) {
	    std::set<socket_type>::const_iterator all_it=all_sockets.begin(), all_end=all_sockets.end();
	    Probe::ready_type ready_bits;

	    for (; all_it!=all_end; ++all_it) {
		ready_bits = Probe::ready_none;

		if (FD_ISSET(*all_it, &rd_fdset)) ready_bits |= Probe::ready_read;
		if (FD_ISSET(*all_it, &wr_fdset)) ready_bits |= Probe::ready_write;
		if (FD_ISSET(*all_it, &er_fdset)) ready_bits |= Probe::ready_oobd;
		if (ready_bits != Probe::ready_none) return_value.push_back(std::make_pair(*all_it, ready_bits));
	    }

	    return return_value;

	} else if (rc == 0) {
	    return return_value;
	} else {
	    error_type error_code = get_last_error();

	    switch (error_code) {
		case EINTR:
		    continue;

		default:
		{
		    std::string error("select(2): ");
		    error += strerror(error_code);
		    throw Exception(error);
		}
	    }
	}
    }

    /* not reached */
    return return_value;
}
//####################################################################
