#include <iostream>
#include <iomanip>
#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST
#include "test.hxx"

namespace s2c {
namespace test {



// Classes for PeerIdPdu */

void PeerIdPdu :: print(std::ostream *out, int indent)
{
   do_indent(out,indent);
   (*out) << "peer_id:\n";
   indent+=2;
   for(int i=0;i<16;i++) {
      do_indent(out, indent);
   (*out)  << "opaque:" << std::hex << (unsigned long long) mId[i] << "\n"; 
   }
};

void PeerIdPdu :: decode(std::istream *in)
{
 DebugLog(<< "Decoding PeerIdPdu");
   for(int i=0;i<16;i++)
      decode_uintX(in, 8, mId[i]);
 DebugLog( << "mId[i]");

};

void PeerIdPdu :: encode(std::ostream *out)
{
   DebugLog(<< "Encoding PeerIdPdu");
   for(int i=0;i<16;i++)
      encode_uintX(out, 8, mId[i]);

};



// Classes for BazTypePdu */

void BazTypePdu :: print(std::ostream *out, int indent)
{
   do_indent(out,indent);
   (*out) << "baz_type:\n";
   indent+=2;
   do_indent(out, indent);
   (*out)  << "zzz:" << std::hex << (unsigned long long)mZzz << "\n"; 
};

void BazTypePdu :: decode(std::istream *in)
{
 DebugLog(<< "Decoding BazTypePdu");
   decode_uintX(in, 16, mZzz);
 DebugLog( << "mZzz");

};

void BazTypePdu :: encode(std::ostream *out)
{
   DebugLog(<< "Encoding BazTypePdu");
   encode_uintX(out, 16, mZzz);

};



// Classes for FooPdu */

void FooPdu :: print(std::ostream *out, int indent)
{
   do_indent(out,indent);
   (*out) << "foo:\n";
   indent+=2;
   do_indent(out, indent);
   (*out)  << "bar:" << std::hex << (unsigned long long)mBar << "\n"; 
   for(int i=0;i<mVariable.size();i++){
      do_indent(out, indent);
   (*out)  << "uint8:" << std::hex << (unsigned long long) mVariable[i] << "\n"; 
   }
   mMumble->print(out, indent);
   mId->print(out, indent);
   do_indent(out, indent);
   (*out)  << "zulu:" << std::hex << (unsigned long long)mZulu << "\n"; 
   for(int i=0;i<5;i++) {
      do_indent(out, indent);
   (*out)  << "uint8:" << std::hex << (unsigned long long) mWww[i] << "\n"; 
   }
   for(int i=0;i<3;i++) {
      do_indent(out, indent);
   (*out)  << "uint16:" << std::hex << (unsigned long long) mWwx[i] << "\n"; 
   }
};

void FooPdu :: decode(std::istream *in)
{
 DebugLog(<< "Decoding FooPdu");
   decode_uintX(in, 8, mBar);
 DebugLog( << "mBar");

   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mVariable.push_back(0);
      decode_uintX(&in2, 8, mVariable[i++]);
 DebugLog( << "mVariable[i++]");
   }
;
   mMumble = new BazTypePdu();
   mMumble->decode(in);

   mId = new PeerIdPdu();
   mId->decode(in);

   decode_uintX(in, 16, mZulu);
 DebugLog( << "mZulu");

   for(int i=0;i<5;i++)
      decode_uintX(in, 8, mWww[i]);
 DebugLog( << "mWww[i]");

   for(int i=0;i<3;i++)
      decode_uintX(in, 16, mWwx[i]);
 DebugLog( << "mWwx[i]");

};

void FooPdu :: encode(std::ostream *out)
{
   DebugLog(<< "Encoding FooPdu");
   encode_uintX(out, 8, mBar);

   long pos1=out->tellp();
   out->seekp(pos1 + 2);
   for(int i=0;i<mVariable.size();i++)
      encode_uintX(out, 8, mVariable[i]);
   long pos2=out->tellp();
   out->seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out->seekp(pos2);

   mMumble->encode(out);

   mId->encode(out);

   encode_uintX(out, 16, mZulu);

   for(int i=0;i<5;i++)
      encode_uintX(out, 8, mWww[i]);

   for(int i=0;i<3;i++)
      encode_uintX(out, 16, mWwx[i]);

};

}
}
