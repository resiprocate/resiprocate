
#include <iostream>
#include <sstream>

#include "rutil/Random.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/dum/PublicationPersistenceManager.hxx"

using namespace resip;
using namespace std;


int main(int argc, const char* argv[])
{
    Uri uri("sip:pub@test.com");

    Pidf pidf;
    pidf.setSimpleStatus(true);
    pidf.setEntity(uri);
    pidf.setSimpleId("a75e5e0fb2cf");

    Data eventType("presence");
    Data aorString(uri.getAor());
    Data eTag("e64c1aa043680d6b");
    UInt64 now = Timer::getTimeSecs();
    UInt64 expirationTime = now + 120;

    //cout << "Contents size:" << pidf.getBodyData().size() << endl;
    //cout << pidf.getBodyData() << endl;

    //cout << "Contents xml escaped size:" << pidf.getBodyData().xmlCharDataEncode().size() << endl;
    //cout << pidf.getBodyData().xmlCharDataEncode() << endl;

    Data contentData = pidf.getBodyData();
    Mime mime = pidf.getType();
    Contents* contents = Contents::createContents(mime, contentData);
    cout << "Reconstructed pidf: " << contents->getBodyData() << endl;
    delete contents;
    contents = 0;

    PublicationPersistenceManager::PubDocument doc(eventType, aorString, eTag, expirationTime, &pidf, 0);
    doc.mLastUpdated = now;

    Data docData;
    DataStream dStream(docData);
    doc.stream(dStream);
    dStream.flush();

    Data expectedDoc =
        "<pubinfo>\r\n"
        "  <eventtype>presence</eventtype>\r\n"
        "  <documentkey>pub@test.com</documentkey>\r\n"
        "  <etag>e64c1aa043680d6b</etag>\r\n"
        "  <expires>120</expires>\r\n"
        "  <lastupdate>0</lastupdate>\r\n"
        "  <contentstype>application</contentstype>\r\n"
        "  <contentssubtype>pidf+xml</contentssubtype>\r\n"
        "  <contents>&lt;?xml version=&quot;1.0&quot; encoding=&quot;UTF-8&quot;?&gt;\r\n"
        "&lt;presence xmlns=&quot;urn:ietf:params:xml:ns:pidf&quot;\r\n"
        "          entity=&quot;sip:pub@test.com&quot;&gt;\r\n"
        "  &lt;tuple id=&quot;a75e5e0fb2cf&quot; &gt;\r\n"
        "     &lt;status&gt;&lt;basic&gt;open&lt;/basic&gt;&lt;/status&gt;\r\n"
        "  &lt;/tuple&gt;\r\n"
        "&lt;/presence&gt;\r\n"
        "</contents>\r\n"
        "</pubinfo>\r\n"
    ;

    if(docData != expectedDoc)
    {
        cout << "docData size:" << docData.size() << endl;
        cout << docData << endl;

        cout << "expectedDoc size:" << expectedDoc.size() << endl;
        cout << expectedDoc << endl;

        size_t charIndex = 0;
        while(charIndex < expectedDoc.size() && charIndex < docData.size())
        {
            if(docData[charIndex] != expectedDoc[charIndex])
            {
                cout << "docData[" << charIndex << "] expected: " 
                     << expectedDoc[charIndex] << " (" << (int) expectedDoc[charIndex] << ")"
                     << " is: " << docData[charIndex] << " ("<< (int) docData[charIndex] << ")" <<endl;
            }

            charIndex++;
        }
    }
    assert(docData == expectedDoc);

    ParseBuffer pb(docData);
    XMLCursor xml(pb);

    PublicationPersistenceManager::PubDocument reconstituted;
    assert(reconstituted.deserialize(xml));

    assert(reconstituted.mETag == eTag);
    assert(reconstituted.mEventType == eventType);
    assert(reconstituted.mDocumentKey == aorString);
    assert(reconstituted.mExpirationTime = expirationTime);

    assert(reconstituted.mContents->getContents());
    cout << "Content type: " << reconstituted.mContents->getType() << endl;
    cout << "Content: " << reconstituted.mContents->getBodyData() << endl;
   
    // It seems Pidf and GenericPidfContents both register the exact same Mime type.
    // So the factory give us back a GenericPidfContents instead of a Pidf 
    GenericPidfContents* streamedPidf = dynamic_cast<GenericPidfContents*> (reconstituted.mContents->getContents());
    assert(streamedPidf);
    assert(streamedPidf->getSimplePresenceOnline() == true);
    assert(streamedPidf->getEntity() == uri);


    cout << "testPubDocument succeeded" << endl;
    return(0); 
}   


