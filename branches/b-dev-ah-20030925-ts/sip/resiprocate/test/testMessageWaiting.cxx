#include "resiprocate/MessageWaitingContents.hxx"
#include "resiprocate/HeaderFieldValue.hxx"
#include <iostream>

using namespace resip;
using namespace std;

int
main()
{
   {
      const Data txt("s\r\nk");
      ParseBuffer pb(txt.data(), txt.size());
      pb.skipChar();

      assert(*skipSipLWS(pb) == '\r');
   }

   {
      const Data txt("s\r\n\r\nk");
      ParseBuffer pb(txt.data(), txt.size());
      pb.skipChar();

      assert(skipSipLWS(pb) == txt.data()+1);
   }

   {
      const Data txt("Messages-Waiting: yes\r\n"
                     "Message-Account: sip:alice@vmail.example.com\r\n"
                     "Voice-Message: 4/8 (1/2)\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "simple-message-summary");
      MessageWaitingContents mwb(&hfv, type);

      assert(mwb.header(mw_account).scheme() == "sip");
      assert(mwb.header(mw_account).user() == "alice");
      assert(mwb.header(mw_account).host() == "vmail.example.com");
      
      assert(mwb.header(mw_voice).newCount() == 4);
      assert(mwb.header(mw_voice).oldCount() == 8);
      assert(mwb.header(mw_voice).urgent() == true);
      assert(mwb.header(mw_voice).urgentNewCount() == 1);
      assert(mwb.header(mw_voice).urgentOldCount() == 2);

      assert(mwb.exists(mw_fax) == false);
   }

   {
      const Data txt("Messages-Waiting: yes\r\n"
                     "Message-Account: sip:alice@vmail.example.com\r\n"
                     "Voice-Message: 4/8 (1/2)\r\n"
                     "Fax-Message: 2/1\r\n"
                     "\r\n"
                     "Something: fine choice\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("application", "simple-message-summary");
      MessageWaitingContents mwb(&hfv, type);

      assert(mwb.header(mw_account).scheme() == "sip");
      assert(mwb.header(mw_account).user() == "alice");
      assert(mwb.header(mw_account).host() == "vmail.example.com");
      
      assert(mwb.header(mw_voice).newCount() == 4);
      assert(mwb.header(mw_voice).oldCount() == 8);
      assert(mwb.header(mw_voice).urgent() == true);
      assert(mwb.header(mw_voice).urgentNewCount() == 1);
      assert(mwb.header(mw_voice).urgentOldCount() == 2);

      assert(mwb.header(mw_fax).newCount() == 2);
      assert(mwb.header(mw_fax).oldCount() == 1);
      assert(mwb.header(mw_fax).urgent() == false);
      
      assert(mwb.header(Data("Something")) == Data("fine choice"));

      assert(mwb.exists(mw_fax) == true);
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
