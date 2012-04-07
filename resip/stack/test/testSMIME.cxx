#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/PlainContents.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

#include "rutil/Log.hxx"

#include <fstream>

using resip::BaseSecurity;
using resip::Contents;
using resip::Data;
using resip::Log;
using resip::MultipartMixedContents;
using resip::MultipartSignedContents;
using resip::Pkcs7Contents;
using resip::PlainContents;
using resip::Security;
using resip::SignatureStatus;
using resip::h_ContentTransferEncoding;

int main(int argc, char* argv[])
{
#if defined(USE_SSL)
   Log::initialize(Log::Cout, Log::Info, argv[0]);

   if(argc != 4)
   {
      std::cout << "Usage: testSMIME certDirectory sender@example.com receiver@example.com";
      return -1;
   }

   Security security(argv[1], BaseSecurity::StrongestSuite);
   security.preload();

   const Data sender(argv[2]);
   const Data receiver(argv[3]);

   // .bwc. Signed contents
   {
      PlainContents contents("hello");
      contents.header(h_ContentTransferEncoding).value()="binary";

      MultipartSignedContents* mps = security.sign(sender, &contents);

      std::ofstream file("signedContents.out");
      std::ofstream binFile("binaryBlob1.out");
      file << *mps;

      Pkcs7Contents* pkcs = dynamic_cast<Pkcs7Contents*>(mps->parts().back());
      binFile << pkcs->getBodyData();

      SignatureStatus status=resip::SignatureNone;
      Data sender2(sender);
      // .bwc. The return here is just a reference into mps. Don't delete it.
      Contents* verified = security.checkSignature(mps, &sender2, &status);
      assert(verified);
      switch(status)
      {
         case resip::SignatureTrusted:
         case resip::SignatureCATrusted:
            break;
         case resip::SignatureNotTrusted:
         case resip::SignatureSelfSigned:
         case resip::SignatureNone:
         case resip::SignatureIsBad:
         default:
            assert(0);
      }

      assert(sender2=="sip:"+sender);
      assert(verified->getBodyData()=="hello");

      delete mps;
   }

   // .bwc. Encrypted contents
   {
      PlainContents contents("hello");
      contents.header(h_ContentTransferEncoding).value()="binary";

      Pkcs7Contents* pkcs = security.encrypt(&contents,receiver);

      std::ofstream file("binaryBlob2.out");
      file << *pkcs;

      Contents* decrypted = security.decrypt(receiver, pkcs);
      assert(decrypted);

      assert(decrypted->getBodyData()=="hello");

      delete pkcs;
      delete decrypted;
   }

   // .bwc. Signed and Encrypted contents
   {
      PlainContents contents("hello");
      contents.header(h_ContentTransferEncoding).value()="binary";

      MultipartSignedContents* mps = security.signAndEncrypt(sender, &contents, receiver);

      std::ofstream file("signedAndEncryptedContents.out");
      file << *mps;

      std::ofstream blob3("binaryBlob3.out");
      blob3 << *mps->parts().front();

      std::ofstream blob4("binaryBlob4.out");
      blob4 << *mps->parts().front();

      SignatureStatus status=resip::SignatureNone;
      Data sender2(sender);
      // .bwc. The return here is just a reference into mps. Don't delete it.
      Contents* verified = security.checkSignature(mps, &sender2, &status);
      assert(verified);
      switch(status)
      {
         case resip::SignatureTrusted:
         case resip::SignatureCATrusted:
            break;
         case resip::SignatureNotTrusted:
         case resip::SignatureSelfSigned:
         case resip::SignatureNone:
         case resip::SignatureIsBad:
         default:
            assert(0);
      }

      assert(sender2=="sip:"+sender);

      Pkcs7Contents* pkcs=dynamic_cast<Pkcs7Contents*>(verified);

      assert(pkcs);
      Contents* decrypted = security.decrypt(receiver, pkcs);
      assert(decrypted);
      assert(decrypted->getBodyData()=="hello");

      delete mps;
      delete decrypted;
   }

   return 0;
#else
// No SSL
   std::cout << "Compiled without SSL support -- S/MIME Cannot be tested" << std::endl;
   return -1;
#endif
}
