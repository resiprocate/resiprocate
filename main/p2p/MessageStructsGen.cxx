#include <iostream>
#include <iomanip>
#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST
#include "MessageStructsGen.hxx"

namespace s2c {




// Classes for NodeIdStruct */

void NodeIdStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "NodeId:\n";
   indent+=2;
   for(int i=0;i<16;i++) {
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mId[i] << "\n"; 
   }
};

void NodeIdStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding NodeIdStruct");
   for(int i=0;i<16;i++)
      decode_uintX(in, 8, mId[i]);
 DebugLog( << "mId[i]");

};

void NodeIdStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding NodeIdStruct");
   for(int i=0;i<16;i++)
      encode_uintX(out, 8, mId[i]);

};



// Classes for ResourceIdStruct */

void ResourceIdStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "ResourceId:\n";
   indent+=2;
   for(int i=0;i<mId.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mId[i] << "\n"; 
   }
};

void ResourceIdStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ResourceIdStruct");
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mId.push_back(0);
      decode_uintX(&in2, 8, mId[i++]);
 DebugLog( << "mId[i++]");
   }
;
};

void ResourceIdStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ResourceIdStruct");
   long pos1=out->tellp();
   out->seekp(pos1 + 1);
   for(int i=0;i<mId.size();i++)
      encode_uintX(out, 8, mId[i]);
   long pos2=out->tellp();
   out->seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out->seekp(pos2);

};



// Classes for DestinationDataStruct */

void DestinationDataStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DestinationDataStruct");
};



// Classes for DestinationStruct */

void DestinationStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "Destination:\n";
   indent+=2;
   mType->print(out, indent);
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
   mDestinationData->print(out, indent);
};

void DestinationStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DestinationStruct");
   mType = new DestinationTypeStruct();
   mType->decode(in);

   decode_uintX(in, 8, mLength);
 DebugLog( << "mLength");

   mDestinationData = new DestinationDataStruct();
   mDestinationData->decode(in);

};

void DestinationStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DestinationStruct");
   mType->encode(out);

   encode_uintX(out, 8, mLength);

   mDestinationData->encode(out);

};



// Classes for ForwardingHdrStruct */

void ForwardingHdrStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "ForwardingHdr:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "relo_token:" << std::hex << (unsigned long long)mReloToken << "\n"; 
   do_indent(out, indent);
   (out)  << "overlay:" << std::hex << (unsigned long long)mOverlay << "\n"; 
   do_indent(out, indent);
   (out)  << "ttl:" << std::hex << (unsigned long long)mTtl << "\n"; 
   do_indent(out, indent);
   (out)  << "reserved:" << std::hex << (unsigned long long)mReserved << "\n"; 
   do_indent(out, indent);
   (out)  << "fragment:" << std::hex << (unsigned long long)mFragment << "\n"; 
   do_indent(out, indent);
   (out)  << "version:" << std::hex << (unsigned long long)mVersion << "\n"; 
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
   do_indent(out, indent);
   (out)  << "transaction_id:" << std::hex << (unsigned long long)mTransactionId << "\n"; 
   do_indent(out, indent);
   (out)  << "flags:" << std::hex << (unsigned long long)mFlags << "\n"; 
   for(int i=0;i<mViaList.size();i++){
      mViaList[i]->print(out, indent);
   }
   for(int i=0;i<mDestinationList.size();i++){
      mDestinationList[i]->print(out, indent);
   }
   do_indent(out, indent);
   (out)  << "route_log_len_dummy:" << std::hex << (unsigned long long)mRouteLogLenDummy << "\n"; 
};

void ForwardingHdrStruct :: decode(std::istream& in)
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

   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mViaList.push_back(0);
      mViaList[i++] = new TypeStruct();
   mViaList[i++]->decode(&in2);
   }
;
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mDestinationList.push_back(0);
      mDestinationList[i++] = new TypeStruct();
   mDestinationList[i++]->decode(&in2);
   }
;
   decode_uintX(in, 16, mRouteLogLenDummy);
 DebugLog( << "mRouteLogLenDummy");

};

void ForwardingHdrStruct :: encode(std::ostream& out)
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

   long pos1=out->tellp();
   out->seekp(pos1 + 2);
   for(int i=0;i<mViaList.size();i++)
      mViaList[i]->encode(out);
   long pos2=out->tellp();
   out->seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out->seekp(pos2);

   long pos1=out->tellp();
   out->seekp(pos1 + 2);
   for(int i=0;i<mDestinationList.size();i++)
      mDestinationList[i]->encode(out);
   long pos2=out->tellp();
   out->seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out->seekp(pos2);

   encode_uintX(out, 16, mRouteLogLenDummy);

};

}
