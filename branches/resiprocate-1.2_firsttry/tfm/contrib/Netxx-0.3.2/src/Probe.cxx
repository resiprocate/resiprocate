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
 * This file contains the implementation of the Netxx::Probe class.
**/

// common header
#include "common.h"

// Netxx includes
#include <Netxx/Probe.h>
#include <Netxx/Types.h>
#include <Netxx/ProbeInfo.h>
#include "Probe_impl.h"

// standard includes
#include <list>
#include <utility>
#include <memory>

//####################################################################
namespace {
    typedef std::list<std::pair<Netxx::socket_type, Netxx::Probe::ready_type> > ready_return_type;
    typedef std::list<std::pair<Netxx::socket_type, const Netxx::ProbeInfo*> > pi_type;

    Netxx::Probe::result_type find_ready_match (ready_return_type &q, Netxx::Probe::ready_type rt);
}
//####################################################################
struct Netxx::Probe::pimpl {
    ready_return_type ready_queue_;
    pi_type pilist_;
    Probe_impl probe_impl_;
};
//####################################################################
Netxx::Probe::Probe (void) {
    pimpl_ = new pimpl;
}
//####################################################################
Netxx::Probe::Probe (const Probe &other) {
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl);

    pimpl_->ready_queue_    = other.pimpl_->ready_queue_;
    pimpl_->pilist_	    = other.pimpl_->pilist_;
    pimpl_->probe_impl_	    = other.pimpl_->probe_impl_;

    ap.release();
}
//####################################################################
Netxx::Probe& Netxx::Probe::operator= (const Probe &other) {
    Probe tmp(other); swap(tmp);
    return *this;
}
//####################################################################
void Netxx::Probe::swap (Probe &other) {
    pimpl_->ready_queue_.swap(other.pimpl_->ready_queue_);
    pimpl_->pilist_.swap(other.pimpl_->pilist_);
    pimpl_->probe_impl_.swap(other.pimpl_->probe_impl_);
}
//####################################################################
Netxx::Probe::~Probe (void) {
    delete pimpl_;
}
//####################################################################
void Netxx::Probe::clear (void) {
    pimpl_->ready_queue_.clear();
    pimpl_->pilist_.clear();
    pimpl_->probe_impl_.clear();
}
//####################################################################
std::pair<Netxx::socket_type, Netxx::Probe::ready_type> Netxx::Probe::ready (const Timeout &timeout, ready_type rt) {
    result_type result;
    bool none_match=false;

    if (rt != ready_none && !pimpl_->ready_queue_.empty()) {
	result = find_ready_match(pimpl_->ready_queue_, rt);
	if (result.first != -1) return result;
	none_match = true;
    }

    if (pimpl_->ready_queue_.empty() || none_match) {
	pi_type::const_iterator p_it=pimpl_->pilist_.begin(), p_end=pimpl_->pilist_.end();

	for (; p_it!=p_end; ++p_it) {
	    ProbeInfo::pending_type result = p_it->second->check_pending(p_it->first, rt);

	    if (result != ProbeInfo::pending_none) {
		pimpl_->ready_queue_.push_back(std::make_pair(p_it->first, static_cast<ready_type>(result)));
	    }
	}

	Probe_impl::probe_type probe_result = pimpl_->probe_impl_.probe(timeout, rt);
	Probe_impl::probe_type::const_iterator pr_it=probe_result.begin(), pr_end=probe_result.end();
	for (; pr_it!=pr_end; ++pr_it) { pimpl_->ready_queue_.push_back(*pr_it); }
    }

    if (!pimpl_->ready_queue_.empty()) {
	if (rt != ready_none) {
	    result = find_ready_match(pimpl_->ready_queue_, rt);
	} else {
	    result = pimpl_->ready_queue_.front();
	    pimpl_->ready_queue_.erase(pimpl_->ready_queue_.begin());
	}

	return result;
    }

    result.first = -1;
    result.second = ready_none;
    return result;
}
//####################################################################
void Netxx::Probe::add_socket (socket_type socketfd, ready_type rt) {
    pimpl_->probe_impl_.add(socketfd, rt);
}
//####################################################################
void Netxx::Probe::add_socket (const ProbeInfo *pi, socket_type socketfd, ready_type rt) {
    pimpl_->pilist_.push_back(std::make_pair(socketfd, pi));
    pimpl_->probe_impl_.add(socketfd, rt);
}
//####################################################################
void Netxx::Probe::remove_socket (socket_type socketfd) {
    pi_type::iterator i=pimpl_->pilist_.begin(), end=pimpl_->pilist_.end();
    for (; i!=end; ++i) if (i->first == socketfd) { pimpl_->pilist_.erase(i); break; }
    pimpl_->probe_impl_.remove(socketfd);
}
//####################################################################
namespace {

    //####################################################################
    Netxx::Probe::result_type find_ready_match (ready_return_type &q, Netxx::Probe::ready_type rt) {
	Netxx::Probe::result_type result;
	ready_return_type::iterator q_it=q.begin(), q_end=q.end();

	for (; q_it!=q_end; ++q_it) {
	    if (rt & q_it->second) {
		result.first  = q_it->first;
		result.second = q_it->second;

		q.erase(q_it);
		return result;
	    }
	}

	result.first = -1;
	result.second = Netxx::Probe::ready_none;

	return result;
    }
    //####################################################################
    
}
//####################################################################
