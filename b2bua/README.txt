
Introduction
============

This is a reSIProcate based B2BUA library.

The library is libb2bua.so

It is built on top of reSIProcate's dialog usage manager (DUM)

Building
========

In the main reSIProcate directory:

./configure
make b2bua

NOTE:  the command `make' or `make all' will NOT build the B2BUA
  by default.  I have left it out of the default build until
  it has been tested more extensively by the reSIProcate community.

Background and technical notes
==============================

The code has been adapted from a previous project.  Consequently,
the initial import to the reSIProcate repository doesn't fully adhere to
reSIProcate coding standards.  Nonetheless, it is working code,
so a decision was taken to commit the code as-is, without
modification, and then gradually overhaul it in subsequent commits.

Some key points about this code:

- It produces a shared object - to produce an executable, 
  the B2BUA class must be instantiated and put to work.
  Such an example will be added shortly.  A typical
  implementation of a B2BUA must implement the following
  classes:
  - b2bua::CallRoute
  - b2bua::CallHandle
  - b2bua::AuthorizationManager
  - b2bua::B2BUA

- It relies on rtpproxy from http://www.rtpproxy.org to relay
  the media streams.  I have created various patches for
  rtpproxy 0.2:
  - rtpproxy sends notification to the B2BUA on media timeout
  - fix a file descriptor bug
  - timeout on either direction
  The rtpproxy patches are being posted on the rtpproxy mailing
  list shortly.

Daniel Pocock
daniel@readytechnology.co.uk



/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

