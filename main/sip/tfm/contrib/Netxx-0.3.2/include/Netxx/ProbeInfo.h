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
 * This file contains the defintion of the Netxx::ProbeInfo class.
**/

#ifndef _Netxx_ProbeInfo_h_
#define _Netxx_ProbeInfo_h_

// Netxx includes
#include <Netxx/Types.h>

// standard includes
#include <vector>

namespace Netxx {

/**
 * The Netxx::ProbeInfo class is used to provide information to the
 * Netxx::Probe class on how to probe another Netxx object. This
 * implementation of the class does not do much. The entire implementation
 * is inline to make most of it disapper at compile time.
 *
 * For Netxx objects that need custom probing, they will create their own
 * ProbeInfo class and drive from this one.
**/
class ProbeInfo {
public:
    /**
     * The ProbeInfo::PendingType is used to signal what state a socket is
     * in after a pending check.
     */
    enum PendingType {
	pending_none	= 0x000000, ///< The socket is not ready
	pending_read	= 0x000001, ///< The socket is ready for reading
	pending_write	= 0x000002, ///< The socket is ready for writing
	pending_oobd	= 0x000004  ///< The socket is ready for reading OOB data.
    };

    /// a type for combinding different PendingType's
    typedef unsigned int pending_type;

    //####################################################################
    /** 
     * Netxx::ProbeInfo Default constructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ProbeInfo (void)
    { }

    //####################################################################
    /** 
     * Construct a new ProbeInfo using a vector of sockets.
     *
     * @param sockets The list of sockets to use for probing.
     * @author Peter Jones
    **/
    //####################################################################
    explicit ProbeInfo (const std::vector<socket_type> &sockets)
	: sockets_(sockets) { }

    //####################################################################
    /** 
     * Construct a new ProbeInfo using only one socket.
     *
     * @param socketfd The socket to probe for.
     * @author Peter Jones
    **/
    //####################################################################
    explicit ProbeInfo (socket_type socketfd)
    { add_socket(socketfd); }

    //####################################################################
    /** 
     * Copy constructor.
     *
     * @param other The ProbeInfo to copy from.
     * @author Peter Jones
    **/
    //####################################################################
    ProbeInfo (const ProbeInfo &other)
	: sockets_(other.sockets_) { }

    //####################################################################
    /** 
     * Assignment operator.
     *
     * @param other The ProbeInfo to copy from.
     * @return *this.
     * @author Peter Jones
    **/
    //####################################################################
    ProbeInfo& operator= (const ProbeInfo &other)
    { ProbeInfo tmp(other); swap(tmp); return *this; }

    //####################################################################
    /** 
     * Swap this ProbeInfo for another.
     *
     * @param other The ProbeInfo to swap with.
     * @author Peter Jones
    **/
    //####################################################################
    void swap (ProbeInfo &other)
    { sockets_.swap(other.sockets_); }

    //####################################################################
    /** 
     * Class destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    virtual ~ProbeInfo (void)
    { }

    //####################################################################
    /** 
     * Add a socket file descriptor to the list of sockets to probe.
     *
     * @param socketfd The socket file descriptor to add.
     * @author Peter Jones
    **/
    //####################################################################
    void add_socket (socket_type socketfd)
    { sockets_.push_back(socketfd); }

    //####################################################################
    /** 
     * Get the list of sockets that need to be probed.
     *
     * @return A vector that contains all the sockets to be probed.
     * @author Peter Jones
    **/
    //####################################################################
    const std::vector<socket_type>& get_sockets (void) const
    { return sockets_; }

    //####################################################################
    /** 
     * Reset the list of sockets to probe. Note: this will not reset the
     * sockets to probe in the actual Netxx::Probe class. You should call
     * Netxx::Probe::clear() as well.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void clear (void)
    { sockets_.clear(); }

    //####################################################################
    /** 
     * Override this function if you need special pending checks for your
     * socket file descriptors.
     *
     * @return True if Probe should call check_pending().
     * @return False if Probe should not call check_pending().
     * @author Peter Jones
    **/
    //####################################################################
    virtual bool needs_pending_check (void) const
    { return false; }

    //####################################################################
    /** 
     * Override this function to return the correct pending state for the
     * given socket file descriptor. This call should be very fast and
     * should NEVER EVER block!
     *
     * @param socket_type The socket to check pending status on.
     * @param pending_type What type of pending state are we looking for.
     * @return The pending state for the socket.
     * @author Peter Jones
    **/
    //####################################################################
    virtual pending_type check_pending (socket_type, pending_type) const
    { return pending_none; }
private:
    std::vector<socket_type> sockets_;

}; // end ProbeInfo class

} // end Netxx namespace
#endif
