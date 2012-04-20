#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/PlainContents.hxx"

#include <iostream>
#include <fstream>

using namespace resip;
using namespace std;

void
setContentsHeaders(Contents& contents)
{
    contents.header(h_ContentDisposition).value() = "content-disposition-phrase";
    contents.header(h_ContentDescription).value() = "content-description-phrase";
    contents.header(h_ContentTransferEncoding).value() = "content-transfer-encoding-phrase";
    const char *lang[] = { "en_CA","en_GB","en_US","fr_CA","fr_FR","es_ES.iso88591" };
    for(size_t i = 0 ; i < sizeof(lang)/sizeof(*lang); ++i)
        contents.header(h_ContentLanguages).push_back(Token(lang[i]));
}

void
leakCheck(bool verbose)
{

    Data original;
    Data alternate;

    { // WIN32 workaround
        for (int i = 0 ; i < 100 ; ++i)
        {
            original += "Original Body by Fischer. -- ";
            alternate += "Alternate Body by CheapStandinCo. -- ";
        }
    }

#ifndef WIN32
#ifdef RESIP_USE_STL_STREAMS
    ofstream devnull("/dev/null");
    ostream& os(verbose?cout:devnull);
#else
    EncodeStream &os(verbose?resipCout:resipFastNull);
#endif
    assert(os.good());
#endif

    for(int i = 0 ; i < 100 ; ++i)
    {
          Mime type("text", "plain");

          HeaderFieldValue ohfv(original.data(), original.size());
          PlainContents originalContents(ohfv, type);
          setContentsHeaders(originalContents);

          HeaderFieldValue ahfv(alternate.data(),alternate.size());
          PlainContents alternateContents(ahfv, type);
          setContentsHeaders(alternateContents);

#ifndef WIN32
          if (verbose && i == 0)
          {
              originalContents.encodeHeaders(os);
              originalContents.encode(os);
              os << endl;
              alternateContents.encodeHeaders(os);
              alternateContents.encode(os);
              os << endl;
          }
#endif

          // clobber the content-disposition mDisposition variable
          alternateContents = originalContents;

#if 0
          alternateContents.encodeHeaders(os);
          alternateContents.encode(os);
          os << endl;
#endif
    }
}

int
main(int argc, char *argv[])
{
   {
      const Data txt("some plain text");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("text", "plain");
      PlainContents pc(hfv, type);

      cerr << pc.text() << endl;

      assert(pc.text() == "some plain text");
   }

   leakCheck(argc > 1);

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
