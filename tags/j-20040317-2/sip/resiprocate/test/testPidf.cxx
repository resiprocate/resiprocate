#include "resiprocate/Pidf.hxx"
#include <iostream>
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define CRLF "\r\n"

int
main(int argc, char** argv)
{
   //Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   
   {
      // http://www.imppwg.org/ml-archive/IMPP-WG/200204/msg00094.html

      const Data txt("<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
                     "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf:\"" CRLF
                     "xmlns:im=\"urn:ietf:params:xml:ns:cpim-pidf:im\"" CRLF
                     "xmlns:myex=\"http://id.mycompany.com/cpim-presence/\" " CRLF
                     "entity=\"pres:someone@example.com\">" CRLF
                     "    <tuple id=\"mobile-im\" display=\"displayed\">" CRLF
                     "        <status>" CRLF
                     "            <basic>open</basic>" CRLF
                     "            <im:im>busy</im:im>" CRLF
                     "            <myex:location>home</myex:location>" CRLF
                     "        </status>" CRLF
                     "        <contact priority=\"2\">im:someone@mobilecarrier.net</contact>" CRLF
                     "        <note xml:lang=\"en\">Don't Disturb Please!</note>" CRLF
                     "        <note xml:lang=\"fr\">Ne dérangez pas, s'il vous plait</note>" CRLF
                     "        <timestamp>2001-10-27T16:49:29Z</timestamp>" CRLF
                     "    </tuple>" CRLF
                     "    <tuple id=\"email\">" CRLF
                     "        <status>" CRLF
                     "            <basic>open</basic>" CRLF
                     "        </status>" CRLF
                     "        <contact priority=\"1\">mailto:someone@exapmle.com</contact>" CRLF
                     "    </tuple>" CRLF
                     "    <note xml:lang=\"en\">I'll be in Tokyo next week</note>" CRLF
                     "</presence>");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "pidf+xml");
      Pidf pc(&hfv, type);

      cerr << "!! " << pc.getEntity() << endl;
      assert(pc.getEntity() == "pres:someone@example.com");

      assert(pc.getNumTuples() == 2);
      assert(pc.getTuples()[0].id == "mobile-im");
      assert(pc.getTuples()[0].attributes["display"] == "displayed");
      assert(pc.getTuples()[0].status);
      assert(pc.getTuples()[0].contact == "im:someone@mobilecarrier.net");
      assert(pc.getTuples()[0].contactPriority == 2);
      assert(pc.getTuples()[0].note == "Ne dérangez pas, s'il vous plait");
      assert(pc.getTuples()[0].timeStamp == "2001-10-27T16:49:29Z");

      pc.encodeParsed(cerr);
   }

   cerr << "All OK" << endl;
   return 0;
}

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
