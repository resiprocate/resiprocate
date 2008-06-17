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
   for(unsigned int i=0;i<16;i++) {
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mId[i] << "\n"; 
   }
};

void NodeIdStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding NodeIdStruct");
   for(unsigned int i=0;i<16;i++)
      decode_uintX(in, 8, mId[i]);
 DebugLog( << "mId[i]");

};

void NodeIdStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding NodeIdStruct");
   for(unsigned int i=0;i<16;i++)
      encode_uintX(out, 8, mId[i]);

};



// Classes for ResourceIdStruct */

void ResourceIdStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "ResourceId:\n";
   indent+=2;
   for(unsigned int i=0;i<mId.size();i++){
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
      decode_uintX(in2, 8, mId[i++]);
 DebugLog( << "mId[i++]");
   }
;
};

void ResourceIdStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ResourceIdStruct");
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mId.size();i++)
      encode_uintX(out, 8, mId[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);

};



// Classes for DestinationDataStruct */



// Classes for DestinationStruct */

void DestinationStruct :: print(std::ostream& out, int indent) const
{
   do_indent(out,indent);
   (out) << "Destination:\n";
   indent+=2;
   
   //mType->print(out, indent);
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
   mDestinationData->print(out, indent);
};

void DestinationStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DestinationStruct");
};

void DestinationStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DestinationStruct");
   //mType->encode(out);

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
   for(unsigned int i=0;i<mViaList.size();i++){
      mViaList[i]->print(out, indent);
   }
   for(unsigned int i=0;i<mDestinationList.size();i++){
      mDestinationList[i]->print(out, indent);
   }
   do_indent(out, indent);
   (out)  << "route_log_len_dummy:" << std::hex << (unsigned long long)mRouteLogLenDummy << "\n"; 
};

void ForwardingHdrStruct :: decode(std::istream& in)
{
}

void ForwardingHdrStruct :: encode(std::ostream& out)
{
};

}
