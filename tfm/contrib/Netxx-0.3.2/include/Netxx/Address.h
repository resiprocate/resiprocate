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
 * This file contains the definition of the Netxx::Address class.
**/

#ifndef _Netxx_Address_h_
#define _Netxx_Address_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Peer.h>

// standard includes
#include <string>
#include <vector>

namespace Netxx {

/**
 * The Netxx::Address class is used by Netxx as a container for network
 * addresses. An address can be used by a server to specify which local
 * address and port to bind to. An address can also be used by a client to
 * know which remote host to connect to and on which port.
 *
 * In Netxx, the address also specifies other connection related
 * information, such as the connection protocol.
**/
class Address {
public:
    /// address container type
    typedef std::vector<Peer> container_type;

    /// address const_iterator type
    typedef container_type::const_iterator const_iterator;

    //####################################################################
    /** 
     * Netxx::Address class constructor. This constructor is designed to be
     * used by clients wanting the address information about a server. The
     * url parameter can be a host name with or without a protocol, port
     * number or path. Here are some examples: pmade.org pmade.org:80
     * http://pmade.org http://pmade.org:80/index.html.
     *
     * Here is the order in which the port number is determined. (1) from
     * the port part of the hostname as in pmade.org:80, (2) from the given
     * protocol part such as http:// and (3) from the given default port.
     *
     * @param uri A string as described above
     * @param default_port The default port number for this address
     * @param use_ipv6 Allow the use of IPv6 if compiled in.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Address (const char *uri, port_type default_port=0, bool use_ipv6=false);

    //####################################################################
    /** 
     * Netxx::Address class constructor. This constructor is designed to be
     * used by servers when they need to specify the local addresses and
     * ports to which it will bind.
     *
     * After the constructor is called, you will want to add addresses to
     * the address list using one of the appropriate member functions such
     * as add_address.
     *
     * @param use_ipv6 Allow the use of IPv6 if compiled in.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Address (bool use_ipv6=false);

    //####################################################################
    /** 
     * Netxx::Address class destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Address (void);

    //####################################################################
    /** 
     * If the URI that was given in the constructor had a protocol specifier
     * such as http:// this member function will return that string.
     * Otherwise this function will return 0.
     *
     * @return The protcol string like "http" or 0.
     * @author Peter Jones
    **/
    //####################################################################
    const char* get_protocol (void) const;

    //####################################################################
    /** 
     * Get the hostname that was given in the constructor. This may be a raw
     * address if that was given to the constructor.
     *
     * @return The hostname.
     * @author Peter Jones
    **/
    //####################################################################
    const char* get_name (void) const;

    //####################################################################
    /** 
     * Get the URI path information of that was present in the constructor.
     * For example, if the given URI was pmade.org/index.html, this member
     * function will return "/index.html". If no path information was given
     * in the URI, this function will return 0.
     *
     * @return The path string or 0.
     * @author Peter Jones
    **/
    //####################################################################
    const char* get_path (void) const;

    //####################################################################
    /** 
     * This function will return the actual port number that was chosen by
     * the constructor.
     *
     * @return The final port number for this address.
     * @author Peter Jones
    **/
    //####################################################################
    port_type get_port (void) const;

    //####################################################################
    /** 
     * Get an iterator to the first address pair.
     *
     * @return The address begin iterator.
     * @author Peter Jones
    **/
    //####################################################################
    const_iterator begin (void) const;

    //####################################################################
    /** 
     * Get an iterator that points one past the last address pair.
     *
     * @return The address end iterator.
     * @author Peter Jones
    **/
    //####################################################################
    const_iterator end (void) const;

    //####################################################################
    /** 
     * Find out how many addresses are stored in this address class.
     *
     * @return The number of addresses that this class is holding.
     * @author Peter Jones
    **/
    //####################################################################
    size_type size (void) const;

    //####################################################################
    /** 
     * Add another address to the list. The URI parameter follows that of
     * the first constructor.
     *
     * @param uri The URI of the address.
     * @param default_port The default port to use.
     * @author Peter Jones
    **/
    //####################################################################
    void add_address(const char *uri, port_type default_port=0);

    //####################################################################
    /** 
     * Add all local addresses to the address list. This is meant to be used
     * by servers who wish to bind to all local addresses.
     *
     * @param port The port to use for all local addresses.
     * @author Peter Jones
    **/
    //####################################################################
    void add_all_addresses (port_type port);
private:
    std::string		protocol_;
    std::string		name_;
    std::string		path_;
    port_type		port_;
    container_type	addrs_;
    bool		ipv6_;
}; // end Netxx::Address class

} // end Netxx namespace
#endif
