#include <iostream>
#include <iomanip>
#include "rutil/Logger.hxx"
#include "P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P
#include "MessageStructsGen.hxx"
#include <assert.h>

namespace s2c {




// Classes for NodeIdStruct */

NodeIdStruct :: NodeIdStruct ()
{
   mName = "NodeIdStruct";
 DebugLog(<< "Constructing NodeIdStruct");
   for(unsigned int i=0;i<16;i++)
      mId[i]=0;

};

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

ResourceIdStruct :: ResourceIdStruct ()
{
   mName = "ResourceIdStruct";
 DebugLog(<< "Constructing ResourceIdStruct");
   for(unsigned int i=0;i<248;i++)
      mId[i]=0;

};

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
   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mId.push_back(0);
      decode_uintX(in2, 8, mId[i++]);
   DebugLog( << "mId[i++]");
   }
;   }

};

void ResourceIdStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ResourceIdStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mId.size();i++)
      encode_uintX(out, 8, mId[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

};



// Classes for DestinationStruct */

DestinationStruct :: DestinationStruct ()
{
   mName = "DestinationStruct";
 DebugLog(<< "Constructing DestinationStruct");
   mType=(DestinationType)0;

   mLength=0;

   mPeer.mNodeId=0;
   mResource.mResourceId=0;
   for(unsigned int i=0;i<248;i++)
               mCompressed.mCompressedId[i]=0;

};

void DestinationStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "Destination:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
};

void DestinationStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DestinationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(DestinationType)v;
   }

   decode_uintX(in, 8, mLength);
   DebugLog( << "mLength");

   switch(mType){
      case 1:
            mPeer.mNodeId = new NodeIdStruct();
   mPeer.mNodeId->decode(in);
          break;

      case 2:
            mResource.mResourceId = new ResourceIdStruct();
   mResource.mResourceId->decode(in);
          break;

      case 3:
            {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mCompressed.mCompressedId.push_back(0);
               decode_uintX(in2, 8, mCompressed.mCompressedId[i++]);
   DebugLog( << "mCompressed.mCompressedId[i++]");
   }
;   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void DestinationStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DestinationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   encode_uintX(out, 8, mLength);

   switch(mType) {
      case 1:
            mPeer.mNodeId->encode(out);
          break;

      case 2:
            mResource.mResourceId->encode(out);
          break;

      case 3:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mCompressed.mCompressedId.size();i++)
               encode_uintX(out, 8, mCompressed.mCompressedId[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for SignerIdentityStruct */

SignerIdentityStruct :: SignerIdentityStruct ()
{
   mName = "SignerIdentityStruct";
 DebugLog(<< "Constructing SignerIdentityStruct");
   mIdentityType=(SignerIdentityType)0;

   for(unsigned int i=0;i<65520;i++)
      mSignerIdentity[i]=0;

};

void SignerIdentityStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "SignerIdentity:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "identity_type:" << std::hex << (unsigned long long) mIdentityType << "\n"; 
   for(unsigned int i=0;i<mSignerIdentity.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mSignerIdentity[i] << "\n"; 
   }
};

void SignerIdentityStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding SignerIdentityStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mIdentityType=(SignerIdentityType)v;
   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSignerIdentity.push_back(0);
      decode_uintX(in2, 8, mSignerIdentity[i++]);
   DebugLog( << "mSignerIdentity[i++]");
   }
;   }

};

void SignerIdentityStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding SignerIdentityStruct");
   encode_uintX(out, 8, (u_int64)(mIdentityType));

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSignerIdentity.size();i++)
      encode_uintX(out, 8, mSignerIdentity[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for SignatureAndHashAlgorithmStruct */

SignatureAndHashAlgorithmStruct :: SignatureAndHashAlgorithmStruct ()
{
   mName = "SignatureAndHashAlgorithmStruct";
 DebugLog(<< "Constructing SignatureAndHashAlgorithmStruct");
   mSig=0;

   mHash=0;

};

void SignatureAndHashAlgorithmStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "SignatureAndHashAlgorithm:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "sig:" << std::hex << (unsigned long long)mSig << "\n"; 
   do_indent(out, indent);
   (out)  << "hash:" << std::hex << (unsigned long long)mHash << "\n"; 
};

void SignatureAndHashAlgorithmStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding SignatureAndHashAlgorithmStruct");
   decode_uintX(in, 8, mSig);
   DebugLog( << "mSig");

   decode_uintX(in, 8, mHash);
   DebugLog( << "mHash");

};

void SignatureAndHashAlgorithmStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding SignatureAndHashAlgorithmStruct");
   encode_uintX(out, 8, mSig);

   encode_uintX(out, 8, mHash);

};



// Classes for SignatureStruct */

SignatureStruct :: SignatureStruct ()
{
   mName = "SignatureStruct";
 DebugLog(<< "Constructing SignatureStruct");
   mAlgorithm=0;

   mIdentity=0;

   for(unsigned int i=0;i<65520;i++)
      mSignatureValue[i]=0;

};

void SignatureStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "Signature:\n";
   indent+=2;
   mAlgorithm->print(out, indent);
   mIdentity->print(out, indent);
   for(unsigned int i=0;i<mSignatureValue.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mSignatureValue[i] << "\n"; 
   }
};

void SignatureStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding SignatureStruct");
   mAlgorithm = new SignatureAndHashAlgorithmStruct();
   mAlgorithm->decode(in);

   mIdentity = new SignerIdentityStruct();
   mIdentity->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSignatureValue.push_back(0);
      decode_uintX(in2, 8, mSignatureValue[i++]);
   DebugLog( << "mSignatureValue[i++]");
   }
;   }

};

void SignatureStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding SignatureStruct");
   mAlgorithm->encode(out);

   mIdentity->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSignatureValue.size();i++)
      encode_uintX(out, 8, mSignatureValue[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for ForwardingLayerMessageStruct */

ForwardingLayerMessageStruct :: ForwardingLayerMessageStruct ()
{
   mName = "ForwardingLayerMessageStruct";
 DebugLog(<< "Constructing ForwardingLayerMessageStruct");
   mReloToken=0;

   mOverlay=0;

   mTtl=0;

   mReserved=0;

   mFragment=0;

   mVersion=0;

   mLength=0;

   mTransactionId=0;

   mFlags=0;

   for(unsigned int i=0;i<0;i++)
      mViaList[i]=0;

   for(unsigned int i=0;i<0;i++)
      mDestinationList[i]=0;

   mRouteLogLenDummy=0;

   mMessageCode=0;

   for(unsigned int i=0;i<16777192;i++)
      mPayload[i]=0;

   mSig=0;

};

void ForwardingLayerMessageStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ForwardingLayerMessage:\n";
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
   do_indent(out, indent);
   (out)  << "message_code:" << std::hex << (unsigned long long)mMessageCode << "\n"; 
   for(unsigned int i=0;i<mPayload.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mPayload[i] << "\n"; 
   }
   mSig->print(out, indent);
};

void ForwardingLayerMessageStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ForwardingLayerMessageStruct");
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

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mViaList.push_back(0);
      mViaList[i++] = new DestinationStruct();
   mViaList[i++]->decode(in2);
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mDestinationList.push_back(0);
      mDestinationList[i++] = new DestinationStruct();
   mDestinationList[i++]->decode(in2);
   }
;   }

   decode_uintX(in, 16, mRouteLogLenDummy);
   DebugLog( << "mRouteLogLenDummy");

   decode_uintX(in, 16, mMessageCode);
   DebugLog( << "mMessageCode");

   {
   resip::Data d;
   read_varray1(in, 3, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mPayload.push_back(0);
      decode_uintX(in2, 8, mPayload[i++]);
   DebugLog( << "mPayload[i++]");
   }
;   }

   mSig = new SignatureStruct();
   mSig->decode(in);

};

void ForwardingLayerMessageStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ForwardingLayerMessageStruct");
   encode_uintX(out, 8, mReloToken);

   encode_uintX(out, 32, mOverlay);

   encode_uintX(out, 8, mTtl);

   encode_uintX(out, 8, mReserved);

   encode_uintX(out, 16, mFragment);

   encode_uintX(out, 8, mVersion);

   encode_uintX(out, 24, mLength);

   encode_uintX(out, 64, mTransactionId);

   encode_uintX(out, 16, mFlags);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mViaList.size();i++)
      mViaList[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mDestinationList.size();i++)
      mDestinationList[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

   encode_uintX(out, 16, mRouteLogLenDummy);

   encode_uintX(out, 16, mMessageCode);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 3);
   for(unsigned int i=0;i<mPayload.size();i++)
      encode_uintX(out, 8, mPayload[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 24, (pos2 - pos1) - 3);
   out.seekp(pos2);
   }

   mSig->encode(out);

};

}
