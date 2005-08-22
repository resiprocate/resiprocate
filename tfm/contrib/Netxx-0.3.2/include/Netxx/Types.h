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
 * This file defines some common types for Netxx.
**/

#ifndef _Netxx_Types_h_
#define _Netxx_Types_h_

// standard includes
#include <stdexcept>
#include <string>

namespace Netxx {
    
    /// unsigned size type (used for object sizes)
    typedef unsigned int size_type;

    /// signed size type (used for objects with possible negative values)
    typedef signed int signed_size_type;

    /// type for representing port numbers
    typedef unsigned short port_type;

    /// type for representing socket file descriptors
    typedef signed int socket_type;

    /**
     * The Netxx::Exception class is used by the Netxx library to signal
     * some error condition. It is derived from std::runtime_error which is
     * dervied from std::exception. This makes it suitable to only catch
     * std::exception objects if you wish.
    **/
    struct Exception : public std::runtime_error {
	Exception (const std::string &what_arg) :
	    std::runtime_error(what_arg) { }
    }; // end Netxx::Exception

} // end Netxx namespace
#endif
