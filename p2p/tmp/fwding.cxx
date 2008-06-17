#include <iostream>
#include <iomanip>
#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST
#include "fwding.hxx"

namespace s2c {
namespace fwding {



// Classes for ForwardingHdrStruct */

void ForwardingHdrStruct :: print(std::ostream *out, int indent)
{
   do_indent(out,indent);
   (*out) << "ForwardingHdr:\n";
   indent+=2;
   do_indent(out, indent);
   (*out)  << "relo_token:" << std::hex << (unsigned long long)mReloToken << "\n"; 
   do_indent(out, indent);
   (*out)  << "overlay:" << std::hex << (unsigned long long)mOverlay << "\n"; 
   do_indent(out, indent);
   (*out)  << "ttl:" << std::hex << (unsigned long long)mTtl << "\n"; 
   do_indent(out, indent);
   (*out)  << "reserved:" << std::hex << (unsigned long long)mReserved << "\n"; 
   do_indent(out, indent);
   (*out)  << "fragment:" << std::hex << (unsigned long long)mFragment << "\n"; 
   do_indent(out, indent);
   (*out)  << "version:" << std::hex << (unsigned long long)mVersion << "\n"; 
   do_indent(out, indent);
   (*out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
   do_indent(out, indent);
   (*out)  << "transaction_id:" << std::hex << (unsigned long long)mTransactionId << "\n"; 
   do_indent(out, indent);
   (*out)  << "flags:" << std::hex << (unsigned long long)mFlags << "\n"; 
   do_indent(out, indent);
   (*out)  << "via_list_length:" << std::hex << (unsigned long long)mViaListLength << "\n"; 
   do_indent(out, indent);
   (*out)  << "destination_list_length:" << std::hex << (unsigned long long)mDestinationListLength << "\n"; 
};

void ForwardingHdrStruct :: decode(std::istream *in)
{
 DebugLog(<< "Decoding ForwardingHdrStruct");
   decode_uintX(in, 8, mReloToken);
 DebugLog( << "mReloToken");

   decode_uintX(in, 32, mOverlay);
 DebugLog( << "mOverlay");

   decode_uintX(in, 8, mTtl);
 DebugLog( << "mTtl");

   decode_uintX(in, 8, mReserved);
 DebugLog( << "mReserved");

   decode_uintX(in, 16, mFragment);
 DebugLog( << "mFragment");

   decode_uintX(in, 8, mVersion);
 DebugLog( << "mVersion");

   decode_uintX(in, 24, mLength);
 DebugLog( << "mLength");

   decode_uintX(in, 64, mTransactionId);
 DebugLog( << "mTransactionId");

   decode_uintX(in, 16, mFlags);
 DebugLog( << "mFlags");

   decode_uintX(in, 16, mViaListLength);
 DebugLog( << "mViaListLength");

   decode_uintX(in, 16, mDestinationListLength);
 DebugLog( << "mDestinationListLength");

};

void ForwardingHdrStruct :: encode(std::ostream *out)
{
   DebugLog(<< "Encoding ForwardingHdrStruct");
   encode_uintX(out, 8, mReloToken);

   encode_uintX(out, 32, mOverlay);

   encode_uintX(out, 8, mTtl);

   encode_uintX(out, 8, mReserved);

   encode_uintX(out, 16, mFragment);

   encode_uintX(out, 8, mVersion);

   encode_uintX(out, 24, mLength);

   encode_uintX(out, 64, mTransactionId);

   encode_uintX(out, 16, mFlags);

   encode_uintX(out, 16, mViaListLength);

   encode_uintX(out, 16, mDestinationListLength);

};

}
}
