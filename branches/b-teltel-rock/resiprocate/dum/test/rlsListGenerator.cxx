
#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/GenericContents.hxx"
#include "resiprocate/Rlmi.hxx"
#include "resiprocate/Pidf.hxx"
#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/MultipartRelatedContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/CountStream.hxx"


#include <iostream>
#include <fstream>
#include <memory>


using namespace resip;
using namespace std;

int 
main (int argc, char** argv)
{
   {
      const Data txt(
         "--50UBfW7LSCVLtggUPe5z\r\n"
         "Content-Transfer-Encoding: binary\r\n"
         "Content-ID: <nXYxAE@pres.example.com>\r\n"
         "Content-Type: application/rlmi+xml;charset=\"UTF-8\"\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
         "      uri=\"sip:adam-friends@pres.example.com\" version=\"1\"\r\n"
         "      name=\"Buddy List at COM\" fullState=\"true\">\r\n"
         "  <resource uri=\"sip:bob@example.com\" name=\"Bob Smith\">\r\n"
         "    <instance id=\"juwigmtboe\" state=\"active\"\r\n"
         "              cid=\"bUZBsM@pres.example.com\"/>\r\n"
         "  </resource>\r\n"
         "  <resource uri=\"sip:dave@example.com\" name=\"Dave Jones\">\r\n"
         "    <instance id=\"hqzsuxtfyq\" state=\"active\"\r\n"
         "              cid=\"ZvSvkz@pres.example.com\"/>\r\n"
         "  </resource>\r\n"
         "  <resource uri=\"sip:ed@example.net\" name=\"Ed at NET\" />\r\n"
         "  <resource uri=\"sip:adam-friends@example.org\"\r\n"
         "            name=\"My Friends at ORG\" />\r\n"
         "</list>\r\n"
         "\r\n"
         "--50UBfW7LSCVLtggUPe5z\r\n"
         "Content-Transfer-Encoding: binary\r\n"
         "Content-ID: <bUZBsM@pres.example.com>\r\n"
         "Content-Type: application/pidf+xml;charset=\"UTF-8\"\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
         "    entity=\"sip:bob@example.com\">\r\n"
         "  <tuple id=\"sg89ae\">\r\n"
         "    <status>\r\n"
         "      <basic>open</basic>\r\n"
         "    </status>\r\n"
         "    <contact priority=\"1.0\">sip:bob@example.com</contact>\r\n"
         "  </tuple>\r\n"
         "</presence>\r\n"
         "\r\n"
         "--50UBfW7LSCVLtggUPe5z\r\n"
         "Content-Transfer-Encoding: binary\r\n"
         "Content-ID: <ZvSvkz@pres.example.com>\r\n"
         "Content-Type: application/pidf+xml;charset=\"UTF-8\"\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
         "    entity=\"sip:dave@example.com\">\r\n"
         "  <tuple id=\"slie74\">\r\n"
         "    <status>\r\n"
         "      <basic>closed</basic>\r\n"
         "    </status>\r\n"
         "  </tuple>\r\n"
         "</presence>\r\n"
         "\r\n"
         "--50UBfW7LSCVLtggUPe5z--\r\n"
         );

      ofstream outfile;
      outfile.open("c:\\fullHeaders.bytes", ofstream::out | ofstream::trunc | ofstream::binary);
      
      Mime mpr("multipart", "related");
      mpr.param(p_type) = "application/rlmi+xml";
      mpr.param(p_boundary) = "50UBfW7LSCVLtggUPe5z";      
   
      HeaderFieldValue hfv(txt.data(), txt.size());
      MultipartRelatedContents orig(&hfv, mpr);
      
      size_t size;
      {
         CountStream cs(size);
         orig.encode(cs);
      }
      outfile << "Content-Length: " << size << "\r\n";
      orig.encodeHeaders(outfile);      
      orig.encode(outfile);      
      outfile.close();
   }
   
   return 0;
}
