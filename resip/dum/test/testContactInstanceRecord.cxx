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
    assert(CIR.mSyncContact == false);\
    assert(CIR.mUserInfo == (void*) 0x1);\
    assert(*CIR.mUserData == "sip:proxy1.com:6666");

using namespace resip;
using namespace std;

int main(int argc, const char* argv[])
{

    ContactInstanceRecord rec;
    assert(rec.mUserData == 0);

    UInt64 now = Timer::getTimeSecs();
    
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

    cout << "testContactInstanceRecord succeeded" << endl;
    return(0);
}

