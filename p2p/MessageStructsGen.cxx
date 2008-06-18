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



// Classes for IPv4AddrPortStruct */

IPv4AddrPortStruct :: IPv4AddrPortStruct ()
{
   mName = "IPv4AddrPortStruct";
 DebugLog(<< "Constructing IPv4AddrPortStruct");
   mAddr=0;

   mPort=0;

};

void IPv4AddrPortStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "IPv4AddrPort:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "addr:" << std::hex << (unsigned long long)mAddr << "\n"; 
   do_indent(out, indent);
   (out)  << "port:" << std::hex << (unsigned long long)mPort << "\n"; 
};

void IPv4AddrPortStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding IPv4AddrPortStruct");
   decode_uintX(in, 32, mAddr);
   DebugLog( << "mAddr");

   decode_uintX(in, 16, mPort);
   DebugLog( << "mPort");

};

void IPv4AddrPortStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding IPv4AddrPortStruct");
   encode_uintX(out, 32, mAddr);

   encode_uintX(out, 16, mPort);

};



// Classes for IPv6AddrPortStruct */

IPv6AddrPortStruct :: IPv6AddrPortStruct ()
{
   mName = "IPv6AddrPortStruct";
 DebugLog(<< "Constructing IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++)
      mAddr[i]=0;

   mPort=0;

};

void IPv6AddrPortStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "IPv6AddrPort:\n";
   indent+=2;
   for(unsigned int i=0;i<16;i++) {
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mAddr[i] << "\n"; 
   }
   do_indent(out, indent);
   (out)  << "port:" << std::hex << (unsigned long long)mPort << "\n"; 
};

void IPv6AddrPortStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++)
      decode_uintX(in, 8, mAddr[i]);
   DebugLog( << "mAddr[i]");

   decode_uintX(in, 16, mPort);
   DebugLog( << "mPort");

};

void IPv6AddrPortStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++)
      encode_uintX(out, 8, mAddr[i]);

   encode_uintX(out, 16, mPort);

};



// Classes for IpAddressAndPortStruct */

IpAddressAndPortStruct :: IpAddressAndPortStruct ()
{
   mName = "IpAddressAndPortStruct";
 DebugLog(<< "Constructing IpAddressAndPortStruct");
   mType=(AddressType)0;

   mLength=0;

   mIpv4Address.mV4addrPort=0;
   mIpv6Address.mV6addrPort=0;

};

void IpAddressAndPortStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "IpAddressAndPort:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
};

void IpAddressAndPortStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding IpAddressAndPortStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(AddressType)v;
   }

   decode_uintX(in, 8, mLength);
   DebugLog( << "mLength");

   switch(mType){
      case 1:
            mIpv4Address.mV4addrPort = new IPv4AddrPortStruct();
   mIpv4Address.mV4addrPort->decode(in);
          break;

      case 2:
            mIpv6Address.mV6addrPort = new IPv6AddrPortStruct();
   mIpv6Address.mV6addrPort->decode(in);
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void IpAddressAndPortStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding IpAddressAndPortStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   encode_uintX(out, 8, mLength);

   switch(mType) {
      case 1:
            mIpv4Address.mV4addrPort->encode(out);
          break;

      case 2:
            mIpv6Address.mV6addrPort->encode(out);
          break;

       default: /* User error */ 
          assert(1==0);
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



// Classes for ForwardingHeaderStruct */

ForwardingHeaderStruct :: ForwardingHeaderStruct ()
{
   mName = "ForwardingHeaderStruct";
 DebugLog(<< "Constructing ForwardingHeaderStruct");
   mReloToken=0;

   mOverlay=0;

   mTtl=0;

   mReserved=0;

   mFragment=0;

   mVersion=0;

   mLength=0;

   mTransactionId=0;

   mFlags=0;



   mRouteLogLenDummy=0;

   mMessageCode=0;

};

void ForwardingHeaderStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ForwardingHeader:\n";
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
};

void ForwardingHeaderStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ForwardingHeaderStruct");
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

};

void ForwardingHeaderStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ForwardingHeaderStruct");
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

};



// Classes for ForwardingLayerMessageStruct */

ForwardingLayerMessageStruct :: ForwardingLayerMessageStruct ()
{
   mName = "ForwardingLayerMessageStruct";
 DebugLog(<< "Constructing ForwardingLayerMessageStruct");
   mHeader=0;


   mSig=0;

};

void ForwardingLayerMessageStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ForwardingLayerMessage:\n";
   indent+=2;
   mHeader->print(out, indent);
   for(unsigned int i=0;i<mPayload.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mPayload[i] << "\n"; 
   }
   mSig->print(out, indent);
};

void ForwardingLayerMessageStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ForwardingLayerMessageStruct");
   mHeader = new ForwardingHeaderStruct();
   mHeader->decode(in);

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
   mHeader->encode(out);

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



// Classes for ErrorResponseStruct */

ErrorResponseStruct :: ErrorResponseStruct ()
{
   mName = "ErrorResponseStruct";
 DebugLog(<< "Constructing ErrorResponseStruct");
   mErrorCode=0;



};

void ErrorResponseStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ErrorResponse:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "error_code:" << std::hex << (unsigned long long)mErrorCode << "\n"; 
   for(unsigned int i=0;i<mReasonPhrase.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mReasonPhrase[i] << "\n"; 
   }
   for(unsigned int i=0;i<mErrorInfo.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mErrorInfo[i] << "\n"; 
   }
};

void ErrorResponseStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ErrorResponseStruct");
   decode_uintX(in, 16, mErrorCode);
   DebugLog( << "mErrorCode");

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mReasonPhrase.push_back(0);
      decode_uintX(in2, 8, mReasonPhrase[i++]);
   DebugLog( << "mReasonPhrase[i++]");
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mErrorInfo.push_back(0);
      decode_uintX(in2, 8, mErrorInfo[i++]);
   DebugLog( << "mErrorInfo[i++]");
   }
;   }

};

void ErrorResponseStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ErrorResponseStruct");
   encode_uintX(out, 16, mErrorCode);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mReasonPhrase.size();i++)
      encode_uintX(out, 8, mReasonPhrase[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mErrorInfo.size();i++)
      encode_uintX(out, 8, mErrorInfo[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for JoinReqStruct */

JoinReqStruct :: JoinReqStruct ()
{
   mName = "JoinReqStruct";
 DebugLog(<< "Constructing JoinReqStruct");
   mJoiningPeerId=0;


};

void JoinReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "JoinReq:\n";
   indent+=2;
   mJoiningPeerId->print(out, indent);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mOverlaySpecificData[i] << "\n"; 
   }
};

void JoinReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding JoinReqStruct");
   mJoiningPeerId = new NodeIdStruct();
   mJoiningPeerId->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mOverlaySpecificData.push_back(0);
      decode_uintX(in2, 8, mOverlaySpecificData[i++]);
   DebugLog( << "mOverlaySpecificData[i++]");
   }
;   }

};

void JoinReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding JoinReqStruct");
   mJoiningPeerId->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++)
      encode_uintX(out, 8, mOverlaySpecificData[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for JoinAnsStruct */

JoinAnsStruct :: JoinAnsStruct ()
{
   mName = "JoinAnsStruct";
 DebugLog(<< "Constructing JoinAnsStruct");

};

void JoinAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "JoinAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mOverlaySpecificData[i] << "\n"; 
   }
};

void JoinAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding JoinAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mOverlaySpecificData.push_back(0);
      decode_uintX(in2, 8, mOverlaySpecificData[i++]);
   DebugLog( << "mOverlaySpecificData[i++]");
   }
;   }

};

void JoinAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding JoinAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++)
      encode_uintX(out, 8, mOverlaySpecificData[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for LeaveReqStruct */

LeaveReqStruct :: LeaveReqStruct ()
{
   mName = "LeaveReqStruct";
 DebugLog(<< "Constructing LeaveReqStruct");
   mLeavingPeerId=0;


};

void LeaveReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "LeaveReq:\n";
   indent+=2;
   mLeavingPeerId->print(out, indent);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mOverlaySpecificData[i] << "\n"; 
   }
};

void LeaveReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding LeaveReqStruct");
   mLeavingPeerId = new NodeIdStruct();
   mLeavingPeerId->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mOverlaySpecificData.push_back(0);
      decode_uintX(in2, 8, mOverlaySpecificData[i++]);
   DebugLog( << "mOverlaySpecificData[i++]");
   }
;   }

};

void LeaveReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding LeaveReqStruct");
   mLeavingPeerId->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++)
      encode_uintX(out, 8, mOverlaySpecificData[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for RouteQueryReqStruct */

RouteQueryReqStruct :: RouteQueryReqStruct ()
{
   mName = "RouteQueryReqStruct";
 DebugLog(<< "Constructing RouteQueryReqStruct");
   mSendUpdate=(Boolean)0;

   mDestination=0;


};

void RouteQueryReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "RouteQueryReq:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "send_update:" << std::hex << (unsigned long long) mSendUpdate << "\n"; 
   mDestination->print(out, indent);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mOverlaySpecificData[i] << "\n"; 
   }
};

void RouteQueryReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding RouteQueryReqStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mSendUpdate=(Boolean)v;
   }

   mDestination = new DestinationStruct();
   mDestination->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mOverlaySpecificData.push_back(0);
      decode_uintX(in2, 8, mOverlaySpecificData[i++]);
   DebugLog( << "mOverlaySpecificData[i++]");
   }
;   }

};

void RouteQueryReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding RouteQueryReqStruct");
   encode_uintX(out, 8, (u_int64)(mSendUpdate));

   mDestination->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mOverlaySpecificData.size();i++)
      encode_uintX(out, 8, mOverlaySpecificData[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for FramedMessageStruct */

FramedMessageStruct :: FramedMessageStruct ()
{
   mName = "FramedMessageStruct";
 DebugLog(<< "Constructing FramedMessageStruct");
   mType=(FramedMessageType)0;

   mData.mSequence=0;
   mAck.mAckSequence=0;
   mAck.mReceived=0;

};

void FramedMessageStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FramedMessage:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
};

void FramedMessageStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FramedMessageStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(FramedMessageType)v;
   }

   switch(mType){
      case 128:
            decode_uintX(in, 24, mData.mSequence);
   DebugLog( << "mData.mSequence");
            {
   resip::Data d;
   read_varray1(in, 3, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mData.mMessage.push_back(0);
               decode_uintX(in2, 8, mData.mMessage[i++]);
   DebugLog( << "mData.mMessage[i++]");
   }
;   }
          break;

      case 129:
            decode_uintX(in, 24, mAck.mAckSequence);
   DebugLog( << "mAck.mAckSequence");
            decode_uintX(in, 32, mAck.mReceived);
   DebugLog( << "mAck.mReceived");
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void FramedMessageStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FramedMessageStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 128:
            encode_uintX(out, 24, mData.mSequence);
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 3);
   for(unsigned int i=0;i<mData.mMessage.size();i++)
               encode_uintX(out, 8, mData.mMessage[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 24, (pos2 - pos1) - 3);
   out.seekp(pos2);
   }
          break;

      case 129:
            encode_uintX(out, 24, mAck.mAckSequence);
            encode_uintX(out, 32, mAck.mReceived);
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for IceCandidateStruct */

IceCandidateStruct :: IceCandidateStruct ()
{
   mName = "IceCandidateStruct";
 DebugLog(<< "Constructing IceCandidateStruct");

};

void IceCandidateStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "IceCandidate:\n";
   indent+=2;
   for(unsigned int i=0;i<mCandidate.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mCandidate[i] << "\n"; 
   }
};

void IceCandidateStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding IceCandidateStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mCandidate.push_back(0);
      decode_uintX(in2, 8, mCandidate[i++]);
   DebugLog( << "mCandidate[i++]");
   }
;   }

};

void IceCandidateStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding IceCandidateStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mCandidate.size();i++)
      encode_uintX(out, 8, mCandidate[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for ConnectReqAnsStruct */

ConnectReqAnsStruct :: ConnectReqAnsStruct ()
{
   mName = "ConnectReqAnsStruct";
 DebugLog(<< "Constructing ConnectReqAnsStruct");


   mApplication=0;



};

void ConnectReqAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ConnectReqAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mUfrag.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mUfrag[i] << "\n"; 
   }
   for(unsigned int i=0;i<mPassword.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mPassword[i] << "\n"; 
   }
   do_indent(out, indent);
   (out)  << "application:" << std::hex << (unsigned long long)mApplication << "\n"; 
   for(unsigned int i=0;i<mRole.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mRole[i] << "\n"; 
   }
   for(unsigned int i=0;i<mCandidates.size();i++){
      mCandidates[i]->print(out, indent);
   }
};

void ConnectReqAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ConnectReqAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mUfrag.push_back(0);
      decode_uintX(in2, 8, mUfrag[i++]);
   DebugLog( << "mUfrag[i++]");
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mPassword.push_back(0);
      decode_uintX(in2, 8, mPassword[i++]);
   DebugLog( << "mPassword[i++]");
   }
;   }

   decode_uintX(in, 16, mApplication);
   DebugLog( << "mApplication");

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mRole.push_back(0);
      decode_uintX(in2, 8, mRole[i++]);
   DebugLog( << "mRole[i++]");
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mCandidates.push_back(0);
      mCandidates[i++] = new IceCandidateStruct();
   mCandidates[i++]->decode(in2);
   }
;   }

};

void ConnectReqAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ConnectReqAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mUfrag.size();i++)
      encode_uintX(out, 8, mUfrag[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mPassword.size();i++)
      encode_uintX(out, 8, mPassword[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

   encode_uintX(out, 16, mApplication);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mRole.size();i++)
      encode_uintX(out, 8, mRole[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mCandidates.size();i++)
      mCandidates[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for PingReqStruct */

PingReqStruct :: PingReqStruct ()
{
   mName = "PingReqStruct";
 DebugLog(<< "Constructing PingReqStruct");

};

void PingReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "PingReq:\n";
   indent+=2;
   for(unsigned int i=0;i<mRequestedInfo.size();i++){
      do_indent(out, indent);
   (out)  << "uint8:" << std::hex << (unsigned long long) mRequestedInfo[i] << "\n"; 
   }
};

void PingReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding PingReqStruct");
   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mRequestedInfo.push_back(0);
      decode_uintX(in2, 8, mRequestedInfo[i++]);
   DebugLog( << "mRequestedInfo[i++]");
   }
;   }

};

void PingReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding PingReqStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mRequestedInfo.size();i++)
      encode_uintX(out, 8, mRequestedInfo[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

};



// Classes for PingInformationStruct */

PingInformationStruct :: PingInformationStruct ()
{
   mName = "PingInformationStruct";
 DebugLog(<< "Constructing PingInformationStruct");
   mType=(PingInformationType)0;

   mResponsibleSet.mResponsiblePpb=0;
   mNumResources.mNumResources=0;

};

void PingInformationStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "PingInformation:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
};

void PingInformationStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding PingInformationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(PingInformationType)v;
   }

   switch(mType){
      case 1:
            decode_uintX(in, 32, mResponsibleSet.mResponsiblePpb);
   DebugLog( << "mResponsibleSet.mResponsiblePpb");
          break;

      case 2:
            decode_uintX(in, 32, mNumResources.mNumResources);
   DebugLog( << "mNumResources.mNumResources");
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void PingInformationStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding PingInformationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 1:
            encode_uintX(out, 32, mResponsibleSet.mResponsiblePpb);
          break;

      case 2:
            encode_uintX(out, 32, mNumResources.mNumResources);
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for PingAnsStruct */

PingAnsStruct :: PingAnsStruct ()
{
   mName = "PingAnsStruct";
 DebugLog(<< "Constructing PingAnsStruct");
   mResponseId=0;


};

void PingAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "PingAns:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "response_id:" << std::hex << (unsigned long long)mResponseId << "\n"; 
   for(unsigned int i=0;i<mPingInfo.size();i++){
      mPingInfo[i]->print(out, indent);
   }
};

void PingAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding PingAnsStruct");
   decode_uintX(in, 64, mResponseId);
   DebugLog( << "mResponseId");

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mPingInfo.push_back(0);
      mPingInfo[i++] = new PingInformationStruct();
   mPingInfo[i++]->decode(in2);
   }
;   }

};

void PingAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding PingAnsStruct");
   encode_uintX(out, 64, mResponseId);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mPingInfo.size();i++)
      mPingInfo[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for TunnelReqStruct */

TunnelReqStruct :: TunnelReqStruct ()
{
   mName = "TunnelReqStruct";
 DebugLog(<< "Constructing TunnelReqStruct");
   mApplication=0;



};

void TunnelReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "TunnelReq:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "application:" << std::hex << (unsigned long long)mApplication << "\n"; 
   for(unsigned int i=0;i<mDialogId.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mDialogId[i] << "\n"; 
   }
   for(unsigned int i=0;i<mApplicationPdu.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mApplicationPdu[i] << "\n"; 
   }
};

void TunnelReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding TunnelReqStruct");
   decode_uintX(in, 16, mApplication);
   DebugLog( << "mApplication");

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mDialogId.push_back(0);
      decode_uintX(in2, 8, mDialogId[i++]);
   DebugLog( << "mDialogId[i++]");
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 3, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mApplicationPdu.push_back(0);
      decode_uintX(in2, 8, mApplicationPdu[i++]);
   DebugLog( << "mApplicationPdu[i++]");
   }
;   }

};

void TunnelReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding TunnelReqStruct");
   encode_uintX(out, 16, mApplication);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mDialogId.size();i++)
      encode_uintX(out, 8, mDialogId[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 3);
   for(unsigned int i=0;i<mApplicationPdu.size();i++)
      encode_uintX(out, 8, mApplicationPdu[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 24, (pos2 - pos1) - 3);
   out.seekp(pos2);
   }

};



// Classes for DataValueStruct */

DataValueStruct :: DataValueStruct ()
{
   mName = "DataValueStruct";
 DebugLog(<< "Constructing DataValueStruct");
   mExists=(Boolean)0;


};

void DataValueStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "DataValue:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "exists:" << std::hex << (unsigned long long) mExists << "\n"; 
   for(unsigned int i=0;i<mValue.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mValue[i] << "\n"; 
   }
};

void DataValueStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DataValueStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mExists=(Boolean)v;
   }

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mValue.push_back(0);
      decode_uintX(in2, 8, mValue[i++]);
   DebugLog( << "mValue[i++]");
   }
;   }

};

void DataValueStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DataValueStruct");
   encode_uintX(out, 8, (u_int64)(mExists));

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 4);
   for(unsigned int i=0;i<mValue.size();i++)
      encode_uintX(out, 8, mValue[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   out.seekp(pos2);
   }

};



// Classes for ArrayEntryStruct */

ArrayEntryStruct :: ArrayEntryStruct ()
{
   mName = "ArrayEntryStruct";
 DebugLog(<< "Constructing ArrayEntryStruct");
   mIndex=0;

   mValue=0;

};

void ArrayEntryStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ArrayEntry:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "index:" << std::hex << (unsigned long long)mIndex << "\n"; 
   mValue->print(out, indent);
};

void ArrayEntryStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ArrayEntryStruct");
   decode_uintX(in, 32, mIndex);
   DebugLog( << "mIndex");

   mValue = new DataValueStruct();
   mValue->decode(in);

};

void ArrayEntryStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ArrayEntryStruct");
   encode_uintX(out, 32, mIndex);

   mValue->encode(out);

};



// Classes for DictionaryKeyStruct */

DictionaryKeyStruct :: DictionaryKeyStruct ()
{
   mName = "DictionaryKeyStruct";
 DebugLog(<< "Constructing DictionaryKeyStruct");

};

void DictionaryKeyStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "DictionaryKey:\n";
   indent+=2;
   for(unsigned int i=0;i<mKey.size();i++){
      do_indent(out, indent);
   (out)  << "opaque:" << std::hex << (unsigned long long) mKey[i] << "\n"; 
   }
};

void DictionaryKeyStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DictionaryKeyStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKey.push_back(0);
      decode_uintX(in2, 8, mKey[i++]);
   DebugLog( << "mKey[i++]");
   }
;   }

};

void DictionaryKeyStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DictionaryKeyStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mKey.size();i++)
      encode_uintX(out, 8, mKey[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for DictionaryEntryStruct */

DictionaryEntryStruct :: DictionaryEntryStruct ()
{
   mName = "DictionaryEntryStruct";
 DebugLog(<< "Constructing DictionaryEntryStruct");
   mKey=0;

   mValue=0;

};

void DictionaryEntryStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "DictionaryEntry:\n";
   indent+=2;
   mKey->print(out, indent);
   mValue->print(out, indent);
};

void DictionaryEntryStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding DictionaryEntryStruct");
   mKey = new DictionaryKeyStruct();
   mKey->decode(in);

   mValue = new DataValueStruct();
   mValue->decode(in);

};

void DictionaryEntryStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding DictionaryEntryStruct");
   mKey->encode(out);

   mValue->encode(out);

};



// Classes for StoredDataValueStruct */

StoredDataValueStruct :: StoredDataValueStruct ()
{
   mName = "StoredDataValueStruct";
 DebugLog(<< "Constructing StoredDataValueStruct");
   mModel=(DataModel)0;

   mSingleValue.mSingleValueEntry=0;
   mArray.mArrayEntry=0;
   mDictionary.mDictionaryEntry=0;

};

void StoredDataValueStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoredDataValue:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "model:" << std::hex << (unsigned long long) mModel << "\n"; 
};

void StoredDataValueStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoredDataValueStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mModel=(DataModel)v;
   }

   switch(mModel){
      case 1:
            mSingleValue.mSingleValueEntry = new DataValueStruct();
   mSingleValue.mSingleValueEntry->decode(in);
          break;

      case 2:
            mArray.mArrayEntry = new ArrayEntryStruct();
   mArray.mArrayEntry->decode(in);
          break;

      case 3:
            mDictionary.mDictionaryEntry = new DictionaryEntryStruct();
   mDictionary.mDictionaryEntry->decode(in);
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void StoredDataValueStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoredDataValueStruct");
   encode_uintX(out, 8, (u_int64)(mModel));

   switch(mModel) {
      case 1:
            mSingleValue.mSingleValueEntry->encode(out);
          break;

      case 2:
            mArray.mArrayEntry->encode(out);
          break;

      case 3:
            mDictionary.mDictionaryEntry->encode(out);
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for StoredDataStruct */

StoredDataStruct :: StoredDataStruct ()
{
   mName = "StoredDataStruct";
 DebugLog(<< "Constructing StoredDataStruct");
   mLength=0;

   mStorageTime=0;

   mLifetime=0;

   mValue=0;

   mSignature=0;

};

void StoredDataStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoredData:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
   do_indent(out, indent);
   (out)  << "storage_time:" << std::hex << (unsigned long long)mStorageTime << "\n"; 
   do_indent(out, indent);
   (out)  << "lifetime:" << std::hex << (unsigned long long)mLifetime << "\n"; 
   mValue->print(out, indent);
   mSignature->print(out, indent);
};

void StoredDataStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoredDataStruct");
   decode_uintX(in, 32, mLength);
   DebugLog( << "mLength");

   decode_uintX(in, 64, mStorageTime);
   DebugLog( << "mStorageTime");

   decode_uintX(in, 32, mLifetime);
   DebugLog( << "mLifetime");

   mValue = new StoredDataValueStruct();
   mValue->decode(in);

   mSignature = new SignatureStruct();
   mSignature->decode(in);

};

void StoredDataStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoredDataStruct");
   encode_uintX(out, 32, mLength);

   encode_uintX(out, 64, mStorageTime);

   encode_uintX(out, 32, mLifetime);

   mValue->encode(out);

   mSignature->encode(out);

};



// Classes for StoreKindDataStruct */

StoreKindDataStruct :: StoreKindDataStruct ()
{
   mName = "StoreKindDataStruct";
 DebugLog(<< "Constructing StoreKindDataStruct");
   mKind=0;

   mDataModel=(DataModel)0;

   mGenerationCounter=0;


};

void StoreKindDataStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoreKindData:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "kind:" << std::hex << (unsigned long long)mKind << "\n"; 
   do_indent(out, indent);
   (out)  << "data_model:" << std::hex << (unsigned long long) mDataModel << "\n"; 
   do_indent(out, indent);
   (out)  << "generation_counter:" << std::hex << (unsigned long long)mGenerationCounter << "\n"; 
   for(unsigned int i=0;i<mValues.size();i++){
      mValues[i]->print(out, indent);
   }
};

void StoreKindDataStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoreKindDataStruct");
   decode_uintX(in, 32, mKind);
   DebugLog( << "mKind");

   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mDataModel=(DataModel)v;
   }

   decode_uintX(in, 64, mGenerationCounter);
   DebugLog( << "mGenerationCounter");

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mValues.push_back(0);
      mValues[i++] = new StoredDataStruct();
   mValues[i++]->decode(in2);
   }
;   }

};

void StoreKindDataStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoreKindDataStruct");
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 8, (u_int64)(mDataModel));

   encode_uintX(out, 64, mGenerationCounter);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 4);
   for(unsigned int i=0;i<mValues.size();i++)
      mValues[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   out.seekp(pos2);
   }

};



// Classes for StoreReqStruct */

StoreReqStruct :: StoreReqStruct ()
{
   mName = "StoreReqStruct";
 DebugLog(<< "Constructing StoreReqStruct");
   mResource=0;

   mReplicaNumber=0;


};

void StoreReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoreReq:\n";
   indent+=2;
   mResource->print(out, indent);
   do_indent(out, indent);
   (out)  << "replica_number:" << std::hex << (unsigned long long)mReplicaNumber << "\n"; 
   for(unsigned int i=0;i<mKindData.size();i++){
      mKindData[i]->print(out, indent);
   }
};

void StoreReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoreReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   decode_uintX(in, 8, mReplicaNumber);
   DebugLog( << "mReplicaNumber");

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindData.push_back(0);
      mKindData[i++] = new StoreKindDataStruct();
   mKindData[i++]->decode(in2);
   }
;   }

};

void StoreReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoreReqStruct");
   mResource->encode(out);

   encode_uintX(out, 8, mReplicaNumber);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 4);
   for(unsigned int i=0;i<mKindData.size();i++)
      mKindData[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   out.seekp(pos2);
   }

};



// Classes for StoreKindResponseStruct */

StoreKindResponseStruct :: StoreKindResponseStruct ()
{
   mName = "StoreKindResponseStruct";
 DebugLog(<< "Constructing StoreKindResponseStruct");
   mKind=0;

   mGenerationCounter=0;


};

void StoreKindResponseStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoreKindResponse:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "kind:" << std::hex << (unsigned long long)mKind << "\n"; 
   do_indent(out, indent);
   (out)  << "generation_counter:" << std::hex << (unsigned long long)mGenerationCounter << "\n"; 
   for(unsigned int i=0;i<mReplicas.size();i++){
      mReplicas[i]->print(out, indent);
   }
};

void StoreKindResponseStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoreKindResponseStruct");
   decode_uintX(in, 32, mKind);
   DebugLog( << "mKind");

   decode_uintX(in, 64, mGenerationCounter);
   DebugLog( << "mGenerationCounter");

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mReplicas.push_back(0);
      mReplicas[i++] = new NodeIdStruct();
   mReplicas[i++]->decode(in2);
   }
;   }

};

void StoreKindResponseStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoreKindResponseStruct");
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 64, mGenerationCounter);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mReplicas.size();i++)
      mReplicas[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for StoreAnsStruct */

StoreAnsStruct :: StoreAnsStruct ()
{
   mName = "StoreAnsStruct";
 DebugLog(<< "Constructing StoreAnsStruct");

};

void StoreAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoreAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mKindResponses.size();i++){
      mKindResponses[i]->print(out, indent);
   }
};

void StoreAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoreAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i++] = new StoreKindResponseStruct();
   mKindResponses[i++]->decode(in2);
   }
;   }

};

void StoreAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoreAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mKindResponses.size();i++)
      mKindResponses[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for ArrayRangeStruct */

ArrayRangeStruct :: ArrayRangeStruct ()
{
   mName = "ArrayRangeStruct";
 DebugLog(<< "Constructing ArrayRangeStruct");
   mFirst=0;

   mLast=0;

};

void ArrayRangeStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ArrayRange:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "first:" << std::hex << (unsigned long long)mFirst << "\n"; 
   do_indent(out, indent);
   (out)  << "last:" << std::hex << (unsigned long long)mLast << "\n"; 
};

void ArrayRangeStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ArrayRangeStruct");
   decode_uintX(in, 32, mFirst);
   DebugLog( << "mFirst");

   decode_uintX(in, 32, mLast);
   DebugLog( << "mLast");

};

void ArrayRangeStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ArrayRangeStruct");
   encode_uintX(out, 32, mFirst);

   encode_uintX(out, 32, mLast);

};



// Classes for StoredDataSpecifierStruct */

StoredDataSpecifierStruct :: StoredDataSpecifierStruct ()
{
   mName = "StoredDataSpecifierStruct";
 DebugLog(<< "Constructing StoredDataSpecifierStruct");
   mKind=0;

   mModel=(DataModel)0;

   mGeneration=0;

   mLength=0;


};

void StoredDataSpecifierStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoredDataSpecifier:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "kind:" << std::hex << (unsigned long long)mKind << "\n"; 
   do_indent(out, indent);
   (out)  << "model:" << std::hex << (unsigned long long) mModel << "\n"; 
   do_indent(out, indent);
   (out)  << "generation:" << std::hex << (unsigned long long)mGeneration << "\n"; 
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
};

void StoredDataSpecifierStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding StoredDataSpecifierStruct");
   decode_uintX(in, 32, mKind);
   DebugLog( << "mKind");

   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mModel=(DataModel)v;
   }

   decode_uintX(in, 64, mGeneration);
   DebugLog( << "mGeneration");

   decode_uintX(in, 16, mLength);
   DebugLog( << "mLength");

   switch(mModel){
      case 1:
          break;

      case 2:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mArray.mIndices.push_back(0);
               mArray.mIndices[i++] = new ArrayRangeStruct();
   mArray.mIndices[i++]->decode(in2);
   }
;   }
          break;

      case 3:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mDictionary.mKeys.push_back(0);
               mDictionary.mKeys[i++] = new DictionaryKeyStruct();
   mDictionary.mKeys[i++]->decode(in2);
   }
;   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void StoredDataSpecifierStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding StoredDataSpecifierStruct");
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 8, (u_int64)(mModel));

   encode_uintX(out, 64, mGeneration);

   encode_uintX(out, 16, mLength);

   switch(mModel) {
      case 1:
          break;

      case 2:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mArray.mIndices.size();i++)
               mArray.mIndices[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

      case 3:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mDictionary.mKeys.size();i++)
               mDictionary.mKeys[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for FetchReqStruct */

FetchReqStruct :: FetchReqStruct ()
{
   mName = "FetchReqStruct";
 DebugLog(<< "Constructing FetchReqStruct");
   mResource=0;


};

void FetchReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FetchReq:\n";
   indent+=2;
   mResource->print(out, indent);
   for(unsigned int i=0;i<mSpecifiers.size();i++){
      mSpecifiers[i]->print(out, indent);
   }
};

void FetchReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FetchReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSpecifiers.push_back(0);
      mSpecifiers[i++] = new StoredDataSpecifierStruct();
   mSpecifiers[i++]->decode(in2);
   }
;   }

};

void FetchReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FetchReqStruct");
   mResource->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSpecifiers.size();i++)
      mSpecifiers[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for FetchKindResponseStruct */

FetchKindResponseStruct :: FetchKindResponseStruct ()
{
   mName = "FetchKindResponseStruct";
 DebugLog(<< "Constructing FetchKindResponseStruct");
   mKind=0;

   mGeneration=0;


};

void FetchKindResponseStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FetchKindResponse:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "kind:" << std::hex << (unsigned long long)mKind << "\n"; 
   do_indent(out, indent);
   (out)  << "generation:" << std::hex << (unsigned long long)mGeneration << "\n"; 
   for(unsigned int i=0;i<mValues.size();i++){
      mValues[i]->print(out, indent);
   }
};

void FetchKindResponseStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FetchKindResponseStruct");
   decode_uintX(in, 32, mKind);
   DebugLog( << "mKind");

   decode_uintX(in, 64, mGeneration);
   DebugLog( << "mGeneration");

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mValues.push_back(0);
      mValues[i++] = new StoredDataStruct();
   mValues[i++]->decode(in2);
   }
;   }

};

void FetchKindResponseStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FetchKindResponseStruct");
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 64, mGeneration);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 4);
   for(unsigned int i=0;i<mValues.size();i++)
      mValues[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   out.seekp(pos2);
   }

};



// Classes for FetchAnsStruct */

FetchAnsStruct :: FetchAnsStruct ()
{
   mName = "FetchAnsStruct";
 DebugLog(<< "Constructing FetchAnsStruct");

};

void FetchAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FetchAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mKindResponses.size();i++){
      mKindResponses[i]->print(out, indent);
   }
};

void FetchAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FetchAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i++] = new FetchKindResponseStruct();
   mKindResponses[i++]->decode(in2);
   }
;   }

};

void FetchAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FetchAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 4);
   for(unsigned int i=0;i<mKindResponses.size();i++)
      mKindResponses[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   out.seekp(pos2);
   }

};



// Classes for RemoveReqStruct */

RemoveReqStruct :: RemoveReqStruct ()
{
   mName = "RemoveReqStruct";
 DebugLog(<< "Constructing RemoveReqStruct");
   mResource=0;


};

void RemoveReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "RemoveReq:\n";
   indent+=2;
   mResource->print(out, indent);
   for(unsigned int i=0;i<mSpecifiers.size();i++){
      mSpecifiers[i]->print(out, indent);
   }
};

void RemoveReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding RemoveReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSpecifiers.push_back(0);
      mSpecifiers[i++] = new StoredDataSpecifierStruct();
   mSpecifiers[i++]->decode(in2);
   }
;   }

};

void RemoveReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding RemoveReqStruct");
   mResource->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSpecifiers.size();i++)
      mSpecifiers[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for RemoveAnsStruct */

RemoveAnsStruct :: RemoveAnsStruct ()
{
   mName = "RemoveAnsStruct";
 DebugLog(<< "Constructing RemoveAnsStruct");

};

void RemoveAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "RemoveAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mKindResponses.size();i++){
      mKindResponses[i]->print(out, indent);
   }
};

void RemoveAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding RemoveAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i++] = new StoreKindResponseStruct();
   mKindResponses[i++]->decode(in2);
   }
;   }

};

void RemoveAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding RemoveAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mKindResponses.size();i++)
      mKindResponses[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for FindReqStruct */

FindReqStruct :: FindReqStruct ()
{
   mName = "FindReqStruct";
 DebugLog(<< "Constructing FindReqStruct");
   mResource=0;


};

void FindReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FindReq:\n";
   indent+=2;
   mResource->print(out, indent);
   for(unsigned int i=0;i<mKinds.size();i++){
      do_indent(out, indent);
   (out)  << "KindId:" << std::hex << (unsigned long long) mKinds[i] << "\n"; 
   }
};

void FindReqStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FindReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKinds.push_back(0);
      decode_uintX(in2, 32, mKinds[i++]);
   DebugLog( << "mKinds[i++]");
   }
;   }

};

void FindReqStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FindReqStruct");
   mResource->encode(out);

   {
   long pos1=out.tellp();
   out.seekp(pos1 + 1);
   for(unsigned int i=0;i<mKinds.size();i++)
      encode_uintX(out, 32, mKinds[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   out.seekp(pos2);
   }

};



// Classes for FindKindDataStruct */

FindKindDataStruct :: FindKindDataStruct ()
{
   mName = "FindKindDataStruct";
 DebugLog(<< "Constructing FindKindDataStruct");
   mKind=0;

   mClosest=0;

};

void FindKindDataStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FindKindData:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "kind:" << std::hex << (unsigned long long)mKind << "\n"; 
   mClosest->print(out, indent);
};

void FindKindDataStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FindKindDataStruct");
   decode_uintX(in, 32, mKind);
   DebugLog( << "mKind");

   mClosest = new ResourceIdStruct();
   mClosest->decode(in);

};

void FindKindDataStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FindKindDataStruct");
   encode_uintX(out, 32, mKind);

   mClosest->encode(out);

};



// Classes for FindAnsStruct */

FindAnsStruct :: FindAnsStruct ()
{
   mName = "FindAnsStruct";
 DebugLog(<< "Constructing FindAnsStruct");

};

void FindAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "FindAns:\n";
   indent+=2;
   for(unsigned int i=0;i<mResults.size();i++){
      mResults[i]->print(out, indent);
   }
};

void FindAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding FindAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mResults.push_back(0);
      mResults[i++] = new FindKindDataStruct();
   mResults[i++]->decode(in2);
   }
;   }

};

void FindAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding FindAnsStruct");
   {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mResults.size();i++)
      mResults[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }

};



// Classes for TurnServerStruct */

TurnServerStruct :: TurnServerStruct ()
{
   mName = "TurnServerStruct";
 DebugLog(<< "Constructing TurnServerStruct");
   mIteration=0;

   mServerAddress=0;

};

void TurnServerStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "TurnServer:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "iteration:" << std::hex << (unsigned long long)mIteration << "\n"; 
   mServerAddress->print(out, indent);
};

void TurnServerStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding TurnServerStruct");
   decode_uintX(in, 8, mIteration);
   DebugLog( << "mIteration");

   mServerAddress = new IpAddressAndPortStruct();
   mServerAddress->decode(in);

};

void TurnServerStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding TurnServerStruct");
   encode_uintX(out, 8, mIteration);

   mServerAddress->encode(out);

};



// Classes for SipRegistrationStruct */

SipRegistrationStruct :: SipRegistrationStruct ()
{
   mName = "SipRegistrationStruct";
 DebugLog(<< "Constructing SipRegistrationStruct");
   mType=(SipRegistrationType)0;

   mLength=0;


};

void SipRegistrationStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "SipRegistration:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
   do_indent(out, indent);
   (out)  << "length:" << std::hex << (unsigned long long)mLength << "\n"; 
};

void SipRegistrationStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding SipRegistrationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(SipRegistrationType)v;
   }

   decode_uintX(in, 16, mLength);
   DebugLog( << "mLength");

   switch(mType){
      case 1:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSipRegistrationUri.mUri.push_back(0);
               decode_uintX(in2, 8, mSipRegistrationUri.mUri[i++]);
   DebugLog( << "mSipRegistrationUri.mUri[i++]");
   }
;   }
          break;

      case 2:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSipRegistrationRoute.mContactPrefs.push_back(0);
               decode_uintX(in2, 8, mSipRegistrationRoute.mContactPrefs[i++]);
   DebugLog( << "mSipRegistrationRoute.mContactPrefs[i++]");
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSipRegistrationRoute.mDestinationList.push_back(0);
               mSipRegistrationRoute.mDestinationList[i++] = new DestinationStruct();
   mSipRegistrationRoute.mDestinationList[i++]->decode(in2);
   }
;   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void SipRegistrationStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding SipRegistrationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   encode_uintX(out, 16, mLength);

   switch(mType) {
      case 1:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSipRegistrationUri.mUri.size();i++)
               encode_uintX(out, 8, mSipRegistrationUri.mUri[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

      case 2:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSipRegistrationRoute.mContactPrefs.size();i++)
               encode_uintX(out, 8, mSipRegistrationRoute.mContactPrefs[i]);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mSipRegistrationRoute.mDestinationList.size();i++)
               mSipRegistrationRoute.mDestinationList[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for ChordUpdateStruct */

ChordUpdateStruct :: ChordUpdateStruct ()
{
   mName = "ChordUpdateStruct";
 DebugLog(<< "Constructing ChordUpdateStruct");
   mType=(ChordUpdateType)0;


};

void ChordUpdateStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ChordUpdate:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
};

void ChordUpdateStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ChordUpdateStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(ChordUpdateType)v;
   }

   switch(mType){
      case 1:
          break;

      case 2:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mNeighbors.mPredecessors.push_back(0);
               mNeighbors.mPredecessors[i++] = new NodeIdStruct();
   mNeighbors.mPredecessors[i++]->decode(in2);
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mNeighbors.mSuccessors.push_back(0);
               mNeighbors.mSuccessors[i++] = new NodeIdStruct();
   mNeighbors.mSuccessors[i++]->decode(in2);
   }
;   }
          break;

      case 3:
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mFull.mPredecessors.push_back(0);
               mFull.mPredecessors[i++] = new NodeIdStruct();
   mFull.mPredecessors[i++]->decode(in2);
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mFull.mSuccessors.push_back(0);
               mFull.mSuccessors[i++] = new NodeIdStruct();
   mFull.mSuccessors[i++]->decode(in2);
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mFull.mFingers.push_back(0);
               mFull.mFingers[i++] = new NodeIdStruct();
   mFull.mFingers[i++]->decode(in2);
   }
;   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};

void ChordUpdateStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ChordUpdateStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 1:
          break;

      case 2:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mNeighbors.mPredecessors.size();i++)
               mNeighbors.mPredecessors[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mNeighbors.mSuccessors.size();i++)
               mNeighbors.mSuccessors[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

      case 3:
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mFull.mPredecessors.size();i++)
               mFull.mPredecessors[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mFull.mSuccessors.size();i++)
               mFull.mSuccessors[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
            {
   long pos1=out.tellp();
   out.seekp(pos1 + 2);
   for(unsigned int i=0;i<mFull.mFingers.size();i++)
               mFull.mFingers[i]->encode(out);
   long pos2=out.tellp();
   out.seekp(pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   out.seekp(pos2);
   }
          break;

       default: /* User error */ 
          assert(1==0);
   }


};



// Classes for ChordRouteQueryAnsStruct */

ChordRouteQueryAnsStruct :: ChordRouteQueryAnsStruct ()
{
   mName = "ChordRouteQueryAnsStruct";
 DebugLog(<< "Constructing ChordRouteQueryAnsStruct");
   mNextId=0;

};

void ChordRouteQueryAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ChordRouteQueryAns:\n";
   indent+=2;
   mNextId->print(out, indent);
};

void ChordRouteQueryAnsStruct :: decode(std::istream& in)
{
 DebugLog(<< "Decoding ChordRouteQueryAnsStruct");
   mNextId = new NodeIdStruct();
   mNextId->decode(in);

};

void ChordRouteQueryAnsStruct :: encode(std::ostream& out)
{
   DebugLog(<< "Encoding ChordRouteQueryAnsStruct");
   mNextId->encode(out);

};

}
