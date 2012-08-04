
Introduction
============

This is a reSIProcate based B2BUA library.

The library is libb2bua.so

It is built on top of reSIProcate's dialog usage manager (DUM)

Building
========

In the main reSIProcate directory, when you run configure, you
must explicitly request that the library is built:

./configure --with-b2bua

NOTE:  configure does NOT include the B2BUA by default.

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
  Such an example exists in the apps/basicB2BUA directory.  A typical
  implementation of a B2BUA must implement the following
  classes:
  - b2bua::CallRoute
  - b2bua::CallHandle
  - b2bua::AuthorizationManager
  - b2bua::B2BUA

- It relies on rtpproxy from http://www.rtpproxy.org to relay
  the media streams.  There are also various patches for
  rtpproxy 0.2:
  - rtpproxy sends notification to the B2BUA on media timeout
  - fix a file descriptor bug
  - timeout on either direction
  The rtpproxy patches have been posted on the rtpproxy mailing
  list:

   http://lists.rtpproxy.org/pipermail/users/2008-May/000016.html

Daniel Pocock
daniel@pocock.com.au



/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

