#!/usr/bin/python

# This could be made more efficient for high workloads in various ways,
# for example, creating the LDAP connections in a pool and using
# the asynchronous LDAP programming API
# However, this code is fine as-is for many use cases and provides
# a helpful example of how to integrate repro with an external service

# Debian users: please install the package python-ldap

import resip
import ldap
from urlparse import urlparse

def on_load():
    '''Do initialisation when module loads'''
    resip.log_debug('on_load invoked')

def provide_route(method, request_uri, headers, transport_type, body, cookies, new_headers):
    '''Process a request URI and return the target URI(s)'''
    resip.log_debug('request_uri = ' + request_uri)

    _request_uri = urlparse(request_uri)

    routes = list()
    
    # Basic LDAP server parameters:
    server_uri = 'ldaps://ldap.example.org'
    base_dn = "dc=example,dc=org"

    # this domain will be appended to the phone numbers when creating
    # the target URI:
    phone_domain = 'pbx.example.org'

    # urlparse is not great for "sip:" URIs,
    # the user@host portion is in the 'path' element:
    filter = "(&(objectClass=inetOrgPerson)(mail=%s))" % _request_uri.path

    resip.log_debug("Using filter: %s" % filter)

    try:
        con = ldap.initialize(server_uri)

        scope = ldap.SCOPE_SUBTREE
        retrieve_attributes = None
        result_id = con.search(base_dn, scope, filter, retrieve_attributes)
        result_set = []
        while 1:
            timeout = 1
            result_type, result_data = con.result(result_id, 0, None)
            if (result_data == []):
                break
            else:
                if result_type == ldap.RES_SEARCH_ENTRY:
                    result_set.append(result_data)

        if len(result_set) == 0:
            resip.log_debug("No Results.")
            return routes
        for i in range(len(result_set)):
            for entry in result_set[i]:
                if entry[1].has_key('telephoneNumber'):
                    phone = entry[1]['telephoneNumber'][0]
                    routes.append('sip:' + phone + '@' + phone_domain)

    except ldap.LDAPError, error_message:
        resip.log_err("Couldn't Connect. %s " % error_message)
        return (500)

    return routes

# for testing from the command line:
if __name__ == '__main__':
    print provide_route('sip:bob@example.org')


#  ====================================================================
# 
#  Copyright 2013 Daniel Pocock http://danielpocock.com All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
# 
#  1. Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
# 
#  2. Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in
#  the documentation and/or other materials provided with the
#  distribution.
# 
#  3. Neither the name of the author(s) nor the names of any contributors
#  may be used to endorse or promote products derived from this software
#  without specific prior written permission.
# 
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
# 
#  ====================================================================

