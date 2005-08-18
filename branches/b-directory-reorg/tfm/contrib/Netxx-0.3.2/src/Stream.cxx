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
 * This file contains the implementation for the Netxx::Stream.
**/

// common header
#include "common.h"

// Netxx includes
#include "Netxx/Stream.h"
#include "Netxx/ProbeInfo.h"
#include "Socket.h"

// standard includes
#include <memory>
#include <algorithm>

//####################################################################
struct Netxx::Stream::pimpl {
    pimpl (void) {}
    explicit pimpl (socket_type socketfd) : socket_(socketfd) {}
    pimpl (const pimpl &other) : socket_(other.socket_), pi_(other.pi_) {}

    Socket socket_;
    ProbeInfo pi_;
};
//####################################################################
Netxx::Stream::Stream (const Address &address, const Timeout &timeout) 
    : StreamBase(timeout)
{
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl);

    make_connection(pimpl_->socket_, address);
    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Stream::Stream (socket_type socketfd, const Timeout &timeout)
    : StreamBase(timeout)
{
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl(socketfd));

    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Stream::Stream (const char *uri, port_type default_port, const Timeout &timeout) 
    : StreamBase(timeout)
{
    Address addr(uri, default_port);
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl);

    make_connection(pimpl_->socket_, addr);
    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Stream::Stream (const Stream &other)
    : StreamBase(other.get_timeout())
{
    pimpl_ = new pimpl(*(other.pimpl_));
}
//####################################################################
Netxx::Stream& Netxx::Stream::operator= (const Stream &other) {
    Stream tmp(other);
    swap(tmp);

    return *this;
}
//####################################################################
void Netxx::Stream::swap (Stream &other) {
    pimpl_->socket_.swap(other.pimpl_->socket_);
    pimpl_->pi_.swap(other.pimpl_->pi_);
    swap_base(other);
}
//####################################################################
Netxx::Stream::~Stream (void) {
    delete pimpl_;
}
//####################################################################
void Netxx::Stream::close (void) {
    pimpl_->socket_.close();
    pimpl_->pi_.clear();
}
//####################################################################
int Netxx::Stream::get_socketfd (void) const {
    return pimpl_->socket_.get_socketfd();
}
//####################################################################
Netxx::signed_size_type Netxx::Stream::write (const void *buffer, size_type length) {
    return pimpl_->socket_.write(buffer, length, get_timeout());
}
//####################################################################
Netxx::signed_size_type Netxx::Stream::read (void *buffer, size_type length) {
    return pimpl_->socket_.read(buffer, length, get_timeout());
}
//####################################################################
const Netxx::ProbeInfo* Netxx::Stream::get_probe_info (void) const {
    return &(pimpl_->pi_);
}
//####################################################################
