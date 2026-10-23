#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DialogId.hxx"
#include "resip/dum/DialogSetId.hxx"

#include "resip/dum/Profile.hxx"
#include "rutil/Log.hxx"

using namespace resip;
using namespace std;

int 
main (int argc, char** argv)
{
    int level=(int)Log::Debug;
    if (argc > 1 ) level = atoi(argv[1]);

    Log::initialize(Log::Cout, (resip::Log::Level)level, argv[0]);

    DialogSetId a("a","a");
    DialogSetId b("a","b");
    DialogSetId c("b","a");
    DialogSetId d("b","b");
    DialogSetId e("a","a");

    assert( a == a );
    assert( ! (a < a ) );
    assert( a < b );
    assert( ! ( b < a ) );

    assert( a == a );
    assert( ! ( a > a ) );
    assert( b > a );
    assert( ! ( a >  b ) );

    cout << "a.hash()= " << a.hash() << endl;
    cout << "b.hash()= " << b.hash() << endl;
    cout << "e.hash()= " << e.hash() << endl;

    assert( a.hash() != b.hash() );
    assert( a.hash() == a.hash() );
    assert( a.hash() == e.hash() );

    // Swapped or equal parts must not cancel out (they did with XOR)
    assert( b.hash() != c.hash() );
    assert( a.hash() != d.hash() );
    assert( DialogSetId("x", "x").hash() != DialogSetId("y", "y").hash() );
    assert( DialogId(b, "c").hash() != DialogId(c, "b").hash() );
    assert( DialogId(b, "c").hash() == DialogId(DialogSetId("a", "b"), "c").hash() );

    {
       // Hash flooding: each pair collides under Data::hash() from the state
       // the pairs before it leave, so these 4096 Call-IDs share one hash()
       // value.  With Call-ID == tag the old XOR hash was 0 for any id.  The
       // keyed hash must spread both kinds of id out.
       static const char* const blocks[][2] = {
          { "f6aw3fs", "f93onta" }, { "1w7m2tc", "3vd8lmn" }, { "nftuhdl", "8uzwq-f" },
          { "8xzz6oq", "t978ndy" }, { "kjrw56n", "maqv8-t" }, { "ud0x07z", "j7x6n99" },
          { "k4to6wl", "uburvew" }, { "npq8lut", "39t-oef" }, { "7wz8yn0", "uoz43hq" },
          { "rhx0sor", "ve7zbme" }, { "9h4-h7i", "ish3elg" }, { "3zqz6-b", "tey2hxt" },
       };
       const unsigned numBlocks = sizeof(blocks) / sizeof(blocks[0]);
       for (int sameTag = 0; sameTag < 2; ++sameTag)
       {
          HashMap<DialogSetId, int> dialogSets;
          for (unsigned choice = 0; choice < (1u << numBlocks); ++choice)
          {
             Data callId;
             for (unsigned i = 0; i < numBlocks; ++i)
             {
                callId += blocks[i][(choice >> i) & 1];
             }
             dialogSets[DialogSetId(callId, sameTag ? callId : Data("tag"))] = (int)choice;
          }
          assert( dialogSets.size() == (1u << numBlocks) );
          size_t largestBucket = 0;
          for (size_t i = 0; i < dialogSets.bucket_count(); ++i)
          {
             largestBucket = resipMax(largestBucket, dialogSets.bucket_size(i));
          }
          cout << "largest bucket (same tag " << sameTag << ")= " << largestBucket << endl;
          assert( largestBucket < 16 );
       }
    }

    cout << "test passed." << endl;

}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
