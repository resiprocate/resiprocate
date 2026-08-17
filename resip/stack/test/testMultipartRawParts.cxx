#include <cstdlib>
#include <iostream>

#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/PlainContents.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;
using namespace std;

// MultipartMixedContents::getRawFirstPart() has to hand back the first body
// part exactly as it arrived, header block included, because RFC 1847 wants a
// multipart/signed signature checked against those bytes and not against a
// re-encoding of the parsed part.  Re-encoding drops what the parser does not
// model and imposes its own header order, spacing and capitalisation, so a
// signature made by any other implementation fails to verify.
//
// The layouts below are ones a conforming peer really does send; the last one
// is the header block of the S/MIME message captured in SipTortureTests.cxx.
//
// The last three cases pin the part of the contract BaseSecurity and
// EncryptionManager depend on: reading through the const overload keeps the
// bytes, taking mutable access drops them, and an object that never carried a
// buffer of its own has none to give.

static int failures = 0;

static void
check(const char* what, bool cond)
{
   cerr << (cond ? "ok   " : "FAIL ") << what << endl;
   if (!cond) ++failures;
}

static Mime
signedType()
{
   const Data txt("multipart/signed; boundary=BOUND; "
                  "protocol=\"application/pkcs7-signature\"; micalg=sha256\r\n");
   ParseBuffer pb(txt.data(), txt.size());
   Mime m;
   m.parse(pb);
   return m;
}

// Builds a body with the given first-part header block, and returns both the
// body and the bytes of the first part as they sit on the wire.
static Data
build(const Data& headers, const Data& body, Data& firstPartOnTheWire)
{
   firstPartOnTheWire = headers + "\r\n\r\n" + body;

   Data raw("--BOUND\r\n");
   raw += firstPartOnTheWire;
   raw += "\r\n--BOUND\r\n";
   raw += "Content-Type: application/pkcs7-signature\r\n\r\n";
   raw += "not-a-real-signature";
   raw += "\r\n--BOUND--\r\n";
   return raw;
}

static void
roundTrip(const char* label, const Data& headers)
{
   const Data body("transfer 100 to bob");
   Data expected;
   const Data raw = build(headers, body, expected);

   HeaderFieldValue hfv(raw.data(), raw.size());
   MultipartSignedContents mps(hfv, signedType());

   const Data got = mps.getRawFirstPart();
   const bool same = (got == expected);
   check(label, same);
   if (!same)
   {
      cerr << "       expected <" << expected.escaped() << ">" << endl
           << "       got      <" << got.escaped() << ">" << endl;
   }
}

int
main(int, char**)
{
   roundTrip("plain part is handed back unchanged",
             Data("Content-Type: text/plain"));

   roundTrip("MIME-Version survives",
             Data("Content-Type: text/plain\r\nMIME-Version: 1.0"));

   roundTrip("header order is preserved",
             Data("Content-Disposition: session;handling=optional\r\n"
                  "Content-Type: text/plain"));

   roundTrip("spacing after the colon is preserved",
             Data("Content-Type:  text/plain"));

   roundTrip("spacing inside the Content-Type is preserved",
             Data("Content-Type: text/plain; charset=us-ascii"));

   roundTrip("order used by the S/MIME capture in SipTortureTests",
             Data("Content-Type: text/plain\r\n"
                  "Content-Length: 19\r\n"
                  "Content-Disposition: attachment;handling=required"));

   // A part that differs has to read back differently, otherwise the checks
   // above would pass on a stub that always returns the same thing.
   {
      const Data raw1 = [] { Data f; return build(Data("Content-Type: text/plain"),
                                                  Data("transfer 100 to bob"), f); }();
      const Data raw2 = [] { Data f; return build(Data("Content-Type: text/plain"),
                                                  Data("transfer 999 to eve"), f); }();
      HeaderFieldValue h1(raw1.data(), raw1.size());
      HeaderFieldValue h2(raw2.data(), raw2.size());
      MultipartSignedContents a(h1, signedType());
      MultipartSignedContents b(h2, signedType());
      check("different bodies read back differently",
            a.getRawFirstPart() != b.getRawFirstPart());
   }

   // Only the first part is kept, and it stays the first part when later ones
   // change.  Without this, a later widening back to all parts would go
   // unnoticed, and so would an off by one that returned the second.
   {
      auto three = [](const Data& second) {
         Data raw("--BOUND\r\n");
         raw += "Content-Type: text/plain\r\n\r\ntransfer 100 to bob";
         raw += "\r\n--BOUND\r\n";
         raw += "Content-Type: text/plain\r\n\r\n";
         raw += second;
         raw += "\r\n--BOUND\r\n";
         raw += "Content-Type: application/pkcs7-signature\r\n\r\n";
         raw += "not-a-real-signature";
         raw += "\r\n--BOUND--\r\n";
         return raw;
      };
      const Data rawA = three(Data("second part A"));
      const Data rawB = three(Data("second part B"));
      HeaderFieldValue hA(rawA.data(), rawA.size());
      HeaderFieldValue hB(rawB.data(), rawB.size());
      MultipartSignedContents a(hA, signedType());
      MultipartSignedContents b(hB, signedType());
      const MultipartSignedContents& readOnlyA = a;
      check("a three part body parses as three parts", readOnlyA.parts().size() == 3);
      check("the first part is the one handed back",
            a.getRawFirstPart() ==
               Data("Content-Type: text/plain\r\n\r\ntransfer 100 to bob"));
      check("changing a later part leaves the first one alone",
            a.getRawFirstPart() == b.getRawFirstPart());
   }

   // Reading through the const overload has to keep the bytes.  DUM inspects an
   // incoming body before it verifies it (EncryptionManager::Decrypt::decrypt
   // calls isEncrypted() before getContents()), so if inspection dropped the
   // record, checkSignature() would never see the bytes that arrived.
   {
      const Data body("transfer 100 to bob");
      Data expected;
      const Data raw = build(Data("Content-Type: text/plain"), body, expected);
      HeaderFieldValue hfv(raw.data(), raw.size());
      MultipartSignedContents mps(hfv, signedType());

      const MultipartSignedContents& readOnly = mps;
      check("there is a first part to look at", readOnly.parts().size() == 2);
      check("const access keeps the record", mps.getRawFirstPart() == expected);
   }

   // Taking mutable access drops the record: once a caller can change the
   // parts, the recorded bytes no longer describe the object.
   {
      const Data body("transfer 100 to bob");
      Data expected;
      const Data raw = build(Data("Content-Type: text/plain"), body, expected);
      HeaderFieldValue hfv(raw.data(), raw.size());
      MultipartSignedContents mps(hfv, signedType());
      check("record is there before mutable access", !mps.getRawFirstPart().empty());
      mps.parts();
      check("mutable access drops the record", mps.getRawFirstPart().empty());
   }

   // A copy of a body that was already parsed has no record.  The view points
   // into the buffer of the original, and nothing parses a second time, so
   // there is no point at which a view into the copy's own buffer could be
   // established.
   {
      const Data body("transfer 100 to bob");
      Data expected;
      const Data raw = build(Data("Content-Type: text/plain"), body, expected);
      HeaderFieldValue hfv(raw.data(), raw.size());
      MultipartSignedContents mps(hfv, signedType());
      check("the original has a record", !mps.getRawFirstPart().empty());
      MultipartSignedContents copy(mps);
      check("a copy of a parsed body has no record", copy.getRawFirstPart().empty());
   }

   // A copy taken before anything touched the original is the other case, and
   // it does end up with a record.  The copy constructor deliberately does not
   // force the original to parse, so the copy is unparsed as well and holds a
   // buffer of its own; its first access parses that buffer and establishes a
   // view into it.  Same bytes, different buffer.
   {
      const Data body("transfer 100 to bob");
      Data expected;
      const Data raw = build(Data("Content-Type: text/plain"), body, expected);
      HeaderFieldValue hfv(raw.data(), raw.size());
      MultipartSignedContents mps(hfv, signedType());

      // Nothing has read mps at this point, deliberately.
      MultipartSignedContents copy(mps);
      check("a copy of an untouched body records its own bytes",
            copy.getRawFirstPart() == expected);

      // And the original still works afterwards, from its own buffer.
      check("the original is unaffected by the copy",
            mps.getRawFirstPart() == expected);
   }

   // Nothing was parsed, so there is nothing to hand back.
   {
      MultipartSignedContents built;
      built.parts().push_back(new PlainContents("transfer 100 to bob"));
      check("a locally built body has no record", built.getRawFirstPart().empty());
   }

   cerr << (failures == 0 ? "\nall checks passed\n" : "\nFAILURES\n");
   return failures == 0 ? 0 : -1;
}
