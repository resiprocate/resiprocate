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
 * This file contains the defintion of the Netxx::Probe class.
**/

#ifndef _Netxx_Probe_h_
#define _Netxx_Probe_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Timeout.h>
#include <Netxx/ProbeInfo.h>

// standard includes
#include <vector>
#include <utility>

namespace Netxx {

/**
 * The Netxx::Probe class is a wrapper around one of the Netxx probe
 * classes. The reason that we have a wrapper is because most operating
 * systems support select(2) but some also support more advanced APIs like
 * kqueue(2) or /dev/poll.
**/
class Probe {
public:
    /*
     * Bitmask for telling Probe exactly what you want and for testing the
     * return value from Probe::ready().
     */
    enum ReadyType {
	ready_none  = 0x000000, ///< Nothing or Everything depeding on the context
	ready_read  = 0x000001,	///< Readable
	ready_write = 0x000002, ///< Writable
	ready_oobd  = 0x000004  ///< Readable Out-of-band Data
    };

    /// type for holding the bitmask
    typedef unsigned int ready_type;

    /// type returned from ready()
    typedef std::pair<socket_type, ready_type> result_type;

    //####################################################################
    /** 
     * Construct a new Netxx::Probe object.
     *
     * @author Peter Jones
    **/
    //####################################################################
    Probe (void);

    //####################################################################
    /** 
     * Netxx::Probe copy constructor.
     *
     * @param other The other Probe object to copy from.
     * @author Peter Jones
    **/
    //####################################################################
    Probe (const Probe &other);

    //####################################################################
    /** 
     * Netxx::Probe assignment operator.
     *
     * @param other The other Probe object to copy from.
     * @return *this.
     * @author Peter Jones
    **/
    //####################################################################
    Probe& operator= (const Probe &other);

    //####################################################################
    /** 
     * Swap this Probe and another one. Similar to std::swap().
     *
     * @param other The other Probe to swap with.
     * @author Peter Jones
    **/
    //####################################################################
    void swap(Probe &other);

    //####################################################################
    /** 
     * Netxx::Probe destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Probe (void);

    //####################################################################
    /** 
     * Clear the Probe. All objects will be removed from the Probe and it
     * will be in a brand-new like state.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void clear (void);

    //####################################################################
    /** 
     * Preform the probe. This function will block until either some data is
     * ready or the given timeout expires. You may also supply a bitmask for
     * the type of data you want in this probe.
     *
     * @param timeout How long to wait for data. Can be a NULL timeout to block until probe data is avaliable.
     * @param rt A bitmask to control what is returned. ready_none means all data in this context.
     * @return a std::pair where first is the ready socket and second is a bitmask to tell you what it is ready for.
     * @return a std::pair with first set to -1 to signal a timeout.
     * @author Peter Jones
    **/
    //####################################################################
    result_type ready (const Timeout &timeout=Timeout(), ready_type rt=ready_none);

    //####################################################################
    /** 
     * Add an object to the Probe. The object must support the
     * Netxx::ProbeInfo class. All Netxx classes such as Stream and Datagram
     * support the ProbeInfo class.
     *
     * You can optionally give a bitmask that will tell Probe what type of
     * data to probe for. The default is to probe for all data. In this
     * case, ready_none means probe for all data.
     *
     * @param t The object to add to the Probe.
     * @param rt A bitmask that tells Probe what to probe for.
     * @author Peter Jones
    **/
    //####################################################################
    template <typename T> void add (const T &t, ready_type rt=ready_none) {
	// implemented inline to work around bug in MSVC
	const ProbeInfo *pi = t.get_probe_info();
	std::vector<socket_type>::const_iterator i=pi->get_sockets().begin(), end=pi->get_sockets().end();
	for (; i!=end; ++i) { if (pi->needs_pending_check()) add_socket(pi, *i, rt); else add_socket(*i, rt); }
    }

    //####################################################################
    /** 
     * Remove the given object from the Probe.
     *
     * @param t The object to remove from the Probe.
     * @author Peter Jones
    **/
    //####################################################################
    template <typename T> void remove (const T &t) {
	// implemented inline to work around bug in MSVC
	const ProbeInfo *pi = t.get_probe_info();
	std::vector<socket_type>::const_iterator i=pi->get_sockets().begin(), end=pi->get_sockets().end();
	for (; i!=end; ++i) remove_socket(*i);
    }
private:
    void add_socket (socket_type socketfd, ready_type rt);
    void add_socket (const ProbeInfo *pi, socket_type socketfd, ready_type rt);
    void remove_socket (socket_type socketfd);
    struct pimpl; pimpl *pimpl_;
}; // end Probe class

} // end Netxx namespace
#endif
