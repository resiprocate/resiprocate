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
 * This file contains the definition of the Netxx::SockOpt class.
**/

#ifndef _Netxx_SockOpt_h_
#define _Netxx_SockOpt_h_

// Netxx includes
#include <Netxx/Types.h>

namespace Netxx {

/**
 * The Netxx::SockOpt class is used to set general socket options.
**/
class SockOpt {
public:
    //####################################################################
    /** 
     * Construct a new Netxx::SockOpt class and link it with the given
     * socket file descriptor. If you set the revert flag to true, any
     * options that you set for the socket will be reversed when this
     * SockOpt class is destroyed.
     *
     * @param socketfd The socket file descriptor to set options for.
     * @param revert Whether or not to revert in the destructor.
     * @author Peter Jones
    **/
    //####################################################################
    explicit SockOpt (socket_type socketfd, bool revert=false);

    //####################################################################
    /** 
     * Netxx::SockOpt class destructor. The destructor will possibly revert
     * the socket back to its original state if the revert flag was true in
     * the constructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~SockOpt (void);

    //####################################################################
    /** 
     * Set the socket to non-blocking. This will cause the read and write
     * operations to return with a timeout (-1) if the operation could not
     * be fullfilled imediatly.
     *
     * @return True if the socket was set to non-blocking.
     * @return False if there was some error setting the socket to non-blocking
     * @author Peter Jones
    **/
    //####################################################################
    bool set_non_blocking (void);

    //####################################################################
    /** 
     * Set the socket option that allows you to reuse an address/port pair
     * even if the state for that pair is TIMED_WAIT.
     *
     * @return True if the address could be reused.
     * @return False if the address could not be reused.
     * @author Peter Jones
    **/
    //####################################################################
    bool set_reuse_address (void);
private:
    socket_type socket_;
    bool revert_;

    struct pimpl; pimpl *pimpl_;
    SockOpt (const SockOpt&);
    SockOpt& operator= (const SockOpt&);
}; // end Netxx::SockOpt class

} // end Netxx namespace
#endif
