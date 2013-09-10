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
 * This file contains the definition of the Netxx::Timeout class.
**/

#ifndef _Netxx_Timeout_h_
#define _Netxx_Timeout_h_

namespace Netxx {

/**
 * The Netxx::Timeout class is used by the other Netxx classes to specifiy a
 * timeout value. A timeout is broken down into seconds and microseconds.
 * This class also has a default constructor for a null timeout, that is, no
 * timeout.
 *
 * As a side note, just in case you did not know, there are one million
 * microseconds in one second.
**/
class Timeout {
public:
    //####################################################################
    /** 
     * Construct a null timeout. This means that a timeout value should not
     * be used at all. This is the default for most Netxx classes.
     *
     * @author Peter Jones
    **/
    //####################################################################
    Timeout (void) : sec_(0), usec_(0) { }

    //####################################################################
    /** 
     * Construct a Netxx::Timeout with the given length in seconds and
     * optionally the addition of the given microseconds.
     *
     * @param sec The number of seconds for the timeout.
     * @param usec An optional number of microseconds.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Timeout (long sec, long usec=0) : sec_(sec), usec_(usec) {}

    //####################################################################
    /** 
     * Set the seconds part of the timeout.
     *
     * @param sec The new value for the seconds field.
     * @author Peter Jones
    **/
    //####################################################################
    void set_sec (long sec) 
    { sec_ = sec; }

    //####################################################################
    /** 
     * Get the seconds field of the timeout.
     *
     * @return The timeout's seconds field.
     * @author Peter Jones
    **/
    //####################################################################
    long get_sec (void) const
    { return sec_; }

    //####################################################################
    /** 
     * Set the microseconds field for the timeout.
     *
     * @param usec The microseconds to use for this timeout.
     * @author Peter Jones
    **/
    //####################################################################
    void set_usec (long usec)
    { usec_ = usec; }

    //####################################################################
    /** 
     * Get the microseconds field for this timeout.
     *
     * @return The timeout's microseconds field.
     * @author Peter Jones
    **/
    //####################################################################
    long get_usec (void) const
    { return usec_; }

    //####################################################################
    /** 
     * Test to see if this timeout is valid or null.
     *
     * @return True if this timeout has been set with a value.
     * @return False if this timeout is NULL (set to zero).
     * @author Peter Jones
    **/
    //####################################################################
    operator bool (void) const 
    { return sec_ || usec_; }
private:
    long sec_;
    long usec_;
}; // end Netxx::Timeout class

} // end Netxx namespace
#endif
