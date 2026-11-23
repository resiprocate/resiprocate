#include <iostream>
#include <sstream>

#include "resip/dum/ContactInstanceRecord.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Timer.hxx"
#include "rutil/XMLCursor.hxx"

#define ASSERT_CIR_VALUES(CIR, NOW) \
    assert(CIR.mContact == NameAddr("sip:foo@example.com:1234"));\
    assert(CIR.mRegExpires == 3600 + NOW);\
    assert(CIR.mLastUpdated == NOW - 1000);\
    assert(CIR.mReceivedFrom == Tuple("1.1.1.1", 1111, UDP));\
    assert(CIR.mPublicAddress == Tuple("2.2.2.2", 2222, UDP));\
    assert(CIR.mSipPath.front() == NameAddr("sip:path1@3.3.3.1:3331"));\
    assert(CIR.mSipPath.back() == NameAddr("sip:path2@3.3.3.2:3332"));\
    assert(CIR.mInstance == "01:23:45:67:89:ab:cd:ef");\
    assert(CIR.mUserAgent == "TestUA");\
    assert(CIR.mRegId == 5555);\
    assert(CIR.mCallId == "a84b4c76e66710<x>@pc33.example.com");\
    assert(CIR.mCSeq == 314159);\
    assert(CIR.mSyncContact == false);\
    assert(CIR.mUserInfo == (void*) 0x1);\
    assert(*CIR.mUserData == "sip:proxy1.com:6666");

using namespace resip;
using namespace std;

int main(int argc, const char* argv[])
{

    ContactInstanceRecord rec;
    assert(rec.mUserData == 0);

    uint64_t now = Timer::getTimeSecs();
    
    rec.mContact = NameAddr("sip:foo@example.com:1234");
    rec.mRegExpires = 3600 + now;
    rec.mLastUpdated = now - 1000;
    rec.mReceivedFrom = Tuple("1.1.1.1", 1111, UDP);
    rec.mPublicAddress = Tuple("2.2.2.2", 2222, UDP);
    rec.mSipPath.push_back(NameAddr("sip:path1@3.3.3.1:3331"));
    rec.mSipPath.push_back(NameAddr("sip:path2@3.3.3.2:3332"));
    rec.mInstance = "01:23:45:67:89:ab:cd:ef";
    rec.mUserAgent = "TestUA";
    rec.mRegId = 5555;
    rec.mCallId = "a84b4c76e66710<x>@pc33.example.com";
    rec.mCSeq = 314159;
    rec.mSyncContact = false;
    rec.mUserInfo = (void*)0x1;
    rec.mUserData = new Data("sip:proxy1.com:6666");

    ASSERT_CIR_VALUES(rec, now);

    // Test assignment operator
    ContactInstanceRecord rec1;
    rec1 = rec;
    ASSERT_CIR_VALUES(rec1, now);

    // Test copy constructor
    ContactInstanceRecord rec2(rec);
    ASSERT_CIR_VALUES(rec2, now);

    stringstream ss;
    rec.stream(ss);
    cout << ss.str().c_str() << endl;

    Data expectedContactSerialization = 
    "   <contactinfo>\r\n"
    "      <contacturi>&lt;sip:foo@example.com:1234&gt;</contacturi>\r\n"
    "      <expires>3600</expires>\r\n"
    "      <lastupdate>1000</lastupdate>\r\n"
#ifdef NETNS // Tuple serialization different when NETNS is enabled
    "      <receivedfrom>AAAAAAAAAAAAA1cEAAAAAAEBAQE=</receivedfrom>\r\n"
    "      <publicaddress>AAAAAAAAAAAAA64IAAAAAAICAgI=</publicaddress>\r\n"
#else
    "      <receivedfrom>AAAAAAAAAAAAA1cEAQEBAQ==</receivedfrom>\r\n"
    "      <publicaddress>AAAAAAAAAAAAA64IAgICAg==</publicaddress>\r\n"
#endif
    "      <sippath>sip:path1@3.3.3.1:3331</sippath>\r\n"
    "      <sippath>sip:path2@3.3.3.2:3332</sippath>\r\n"
    "      <instance>01:23:45:67:89:ab:cd:ef</instance>\r\n"
    "      <regid>5555</regid>\r\n"
    "      <useragent>TestUA</useragent>\r\n"
    "      <callid>a84b4c76e66710&lt;x&gt;@pc33.example.com</callid>\r\n"
    "      <cseq>314159</cseq>\r\n"
    "      <userdata>sip:proxy1.com:6666</userdata>\r\n"
    "   </contactinfo>\r\n"
    ;

    assert(ss.str().c_str() == expectedContactSerialization);

    Data contactXml(ss.str().c_str());
    ParseBuffer pb(contactXml);
    XMLCursor xml(pb);

    ContactInstanceRecord rec3;
    assert(rec3.deserialize(xml));
    // mUserInfo is not serialized
    rec3.mUserInfo = rec.mUserInfo;

    ASSERT_CIR_VALUES(rec3, now);

    // A record without a Call-ID (one not created from a REGISTER) does not serialize the Call-ID or CSeq,
    // and deserializes with an empty Call-ID
    ContactInstanceRecord rec4;
    rec4.mContact = NameAddr("sip:bar@example.com");
    stringstream ss4;
    rec4.stream(ss4);
    assert(ss4.str().find("<callid>") == string::npos);
    assert(ss4.str().find("<cseq>") == string::npos);
    Data contactXml4(ss4.str().c_str());
    ParseBuffer pb4(contactXml4);
    XMLCursor xml4(pb4);
    ContactInstanceRecord rec5;
    assert(rec5.deserialize(xml4));
    assert(rec5.mCallId.empty());
    assert(rec5.mCSeq == 0);

    cout << "testContactInstanceRecord succeeded" << endl;
    return(0);
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