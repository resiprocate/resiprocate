#include <iostream>
#include <iomanip>
#include "rutil/Logger.hxx"
#include "rutil/ParseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P
#include "MessageStructsGen.hxx"
#include "rutil/ResipAssert.h"

namespace s2c {


/* EKR: Most unprincipled hack of the day */
#define TELLP(a) (static_cast<resip::DataStream &>(a)).tellp()
#define SEEKP(a,b) (static_cast<resip::DataStream &>(a)).seekp(b)


// Classes for NodeIdStruct */

NodeIdStruct :: NodeIdStruct ()
{
   mName = "NodeIdStruct";
 StackLog(<< "Constructing NodeIdStruct");
   mHigh=0;

   mLow=0;

};

NodeIdStruct :: NodeIdStruct (const NodeIdStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "NodeIdStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void NodeIdStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "NodeId:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "high:" << std::hex << (unsigned long long)mHigh << "\n"; 
   do_indent(out, indent);
   (out)  << "low:" << std::hex << (unsigned long long)mLow << "\n"; 
};

void NodeIdStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding NodeIdStruct");
   decode_uintX(in, 64, mHigh);
   StackLog( << "mHigh =" << std::hex << (unsigned long long) mHigh );

   decode_uintX(in, 64, mLow);
   StackLog( << "mLow =" << std::hex << (unsigned long long) mLow );

};

void NodeIdStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding NodeIdStruct");
   StackLog( << "mHigh =" << std::hex << (unsigned long long) mHigh );
   encode_uintX(out, 64, mHigh);

   StackLog( << "mLow =" << std::hex << (unsigned long long) mLow );
   encode_uintX(out, 64, mLow);

};



// Classes for ResourceIdStruct */

ResourceIdStruct :: ResourceIdStruct ()
{
   mName = "ResourceIdStruct";
 StackLog(<< "Constructing ResourceIdStruct");

};

ResourceIdStruct :: ResourceIdStruct (const ResourceIdStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ResourceIdStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void ResourceIdStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ResourceId:\n";
   indent+=2;
    out << mId.hex();
};

void ResourceIdStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding ResourceIdStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mId);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "id",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void ResourceIdStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ResourceIdStruct");
    encode_uintX(out, 8, mId.size());
    out << mId;

};



// Classes for IPv4AddrPortStruct */

IPv4AddrPortStruct :: IPv4AddrPortStruct ()
{
   mName = "IPv4AddrPortStruct";
 StackLog(<< "Constructing IPv4AddrPortStruct");
   mAddr=0;

   mPort=0;

};

IPv4AddrPortStruct :: IPv4AddrPortStruct (const IPv4AddrPortStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "IPv4AddrPortStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding IPv4AddrPortStruct");
   decode_uintX(in, 32, mAddr);
   StackLog( << "mAddr =" << std::hex << (unsigned long long) mAddr );

   decode_uintX(in, 16, mPort);
   StackLog( << "mPort =" << std::hex << (unsigned long long) mPort );

};

void IPv4AddrPortStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding IPv4AddrPortStruct");
   StackLog( << "mAddr =" << std::hex << (unsigned long long) mAddr );
   encode_uintX(out, 32, mAddr);

   StackLog( << "mPort =" << std::hex << (unsigned long long) mPort );
   encode_uintX(out, 16, mPort);

};



// Classes for IPv6AddrPortStruct */

IPv6AddrPortStruct :: IPv6AddrPortStruct ()
{
   mName = "IPv6AddrPortStruct";
 StackLog(<< "Constructing IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++){
      mAddr[i]=0;
}

   mPort=0;

};

IPv6AddrPortStruct :: IPv6AddrPortStruct (const IPv6AddrPortStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "IPv6AddrPortStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++){
      decode_uintX(in, 8, mAddr[i]);
   StackLog( << "mAddr[i] =" << std::hex << (unsigned long long) mAddr[i] );
   }

   decode_uintX(in, 16, mPort);
   StackLog( << "mPort =" << std::hex << (unsigned long long) mPort );

};

void IPv6AddrPortStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding IPv6AddrPortStruct");
   for(unsigned int i=0;i<16;i++){
      StackLog( << "mAddr[i] =" << std::hex << (unsigned long long) mAddr[i] );
   encode_uintX(out, 8, mAddr[i]);
   }

   StackLog( << "mPort =" << std::hex << (unsigned long long) mPort );
   encode_uintX(out, 16, mPort);

};



// Classes for IpAddressAndPortStruct */

IpAddressAndPortStruct :: IpAddressAndPortStruct ()
{
   mName = "IpAddressAndPortStruct";
 StackLog(<< "Constructing IpAddressAndPortStruct");
   mType=(AddressType)0;

   mLength=0;

   mIpv4Address.mV4addrPort=0;
   mIpv6Address.mV6addrPort=0;

};

IpAddressAndPortStruct :: IpAddressAndPortStruct (const IpAddressAndPortStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "IpAddressAndPortStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding IpAddressAndPortStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(AddressType)v;
   }

   decode_uintX(in, 8, mLength);
   StackLog( << "mLength =" << std::hex << (unsigned long long) mLength );

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
          resip_assert(1==0);
   }


};

void IpAddressAndPortStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding IpAddressAndPortStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   StackLog( << "mLength =" << std::hex << (unsigned long long) mLength );
   encode_uintX(out, 8, mLength);

   switch(mType) {
      case 1:
            mIpv4Address.mV4addrPort->encode(out);
          break;

      case 2:
            mIpv6Address.mV6addrPort->encode(out);
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};



// Classes for DestinationStruct */

DestinationStruct :: DestinationStruct ()
{
   mName = "DestinationStruct";
 StackLog(<< "Constructing DestinationStruct");
   mType=(DestinationType)0;


   mPeer.mNodeId=0;
   mResource.mResourceId=0;

};

DestinationStruct :: DestinationStruct (const DestinationStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "DestinationStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void DestinationStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "Destination:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
};

void DestinationStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding DestinationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(DestinationType)v;
   }

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in_auto(d);

   switch(mType){
      case 1:
            mPeer.mNodeId = new NodeIdStruct();
   mPeer.mNodeId->decode(in_auto);
          break;

      case 2:
            mResource.mResourceId = new ResourceIdStruct();
   mResource.mResourceId->decode(in_auto);
          break;

      case 3:
            {
      UInt32 len;
      int c;
      decode_uintX(in_auto, 8, len);
      resip::DataStream strm(mCompressed.mCompressedId);
      while(len--){
        c=in_auto.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "compressed_id",__FILE__,__LINE__);
          strm.put(c);
      };
   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   if(in_auto.peek()!=EOF)
      throw resip::ParseException("Inner encoded value too long",
      "Destination",__FILE__,__LINE__);
   }
};

void DestinationStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding DestinationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   long pos1=TELLP(out);
   for(int i=0;i<1;i++) out.put(0);

   switch(mType) {
      case 1:
            mPeer.mNodeId->encode(out);
          break;

      case 2:
            mResource.mResourceId->encode(out);
          break;

      case 3:
             encode_uintX(out, 8, mCompressed.mCompressedId.size());
    out << mCompressed.mCompressedId;
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   SEEKP(out,pos2);
};



// Classes for SignerIdentityStruct */

SignerIdentityStruct :: SignerIdentityStruct ()
{
   mName = "SignerIdentityStruct";
 StackLog(<< "Constructing SignerIdentityStruct");
   mIdentityType=(SignerIdentityType)0;


};

SignerIdentityStruct :: SignerIdentityStruct (const SignerIdentityStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "SignerIdentityStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void SignerIdentityStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "SignerIdentity:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "identity_type:" << std::hex << (unsigned long long) mIdentityType << "\n"; 
    out << mSignerIdentity.hex();
};

void SignerIdentityStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding SignerIdentityStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mIdentityType=(SignerIdentityType)v;
   }

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mSignerIdentity);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "signer_identity",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void SignerIdentityStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding SignerIdentityStruct");
   encode_uintX(out, 8, (u_int64)(mIdentityType));

    encode_uintX(out, 16, mSignerIdentity.size());
    out << mSignerIdentity;

};



// Classes for SignatureAndHashAlgorithmStruct */

SignatureAndHashAlgorithmStruct :: SignatureAndHashAlgorithmStruct ()
{
   mName = "SignatureAndHashAlgorithmStruct";
 StackLog(<< "Constructing SignatureAndHashAlgorithmStruct");
   mSig=0;

   mHash=0;

};

SignatureAndHashAlgorithmStruct :: SignatureAndHashAlgorithmStruct (const SignatureAndHashAlgorithmStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "SignatureAndHashAlgorithmStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding SignatureAndHashAlgorithmStruct");
   decode_uintX(in, 8, mSig);
   StackLog( << "mSig =" << std::hex << (unsigned long long) mSig );

   decode_uintX(in, 8, mHash);
   StackLog( << "mHash =" << std::hex << (unsigned long long) mHash );

};

void SignatureAndHashAlgorithmStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding SignatureAndHashAlgorithmStruct");
   StackLog( << "mSig =" << std::hex << (unsigned long long) mSig );
   encode_uintX(out, 8, mSig);

   StackLog( << "mHash =" << std::hex << (unsigned long long) mHash );
   encode_uintX(out, 8, mHash);

};



// Classes for SignatureStruct */

SignatureStruct :: SignatureStruct ()
{
   mName = "SignatureStruct";
 StackLog(<< "Constructing SignatureStruct");
   mAlgorithm=0;

   mIdentity=0;


};

SignatureStruct :: SignatureStruct (const SignatureStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "SignatureStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void SignatureStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "Signature:\n";
   indent+=2;
   mAlgorithm->print(out, indent);
   mIdentity->print(out, indent);
    out << mSignatureValue.hex();
};

void SignatureStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding SignatureStruct");
   mAlgorithm = new SignatureAndHashAlgorithmStruct();
   mAlgorithm->decode(in);

   mIdentity = new SignerIdentityStruct();
   mIdentity->decode(in);

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mSignatureValue);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "signature_value",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void SignatureStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding SignatureStruct");
   mAlgorithm->encode(out);

   mIdentity->encode(out);

    encode_uintX(out, 16, mSignatureValue.size());
    out << mSignatureValue;

};



// Classes for ForwardingHeaderStruct */

ForwardingHeaderStruct :: ForwardingHeaderStruct ()
{
   mName = "ForwardingHeaderStruct";
 StackLog(<< "Constructing ForwardingHeaderStruct");
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

ForwardingHeaderStruct :: ForwardingHeaderStruct (const ForwardingHeaderStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ForwardingHeaderStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding ForwardingHeaderStruct");
   decode_uintX(in, 32, mReloToken);
   StackLog( << "mReloToken =" << std::hex << (unsigned long long) mReloToken );

   decode_uintX(in, 32, mOverlay);
   StackLog( << "mOverlay =" << std::hex << (unsigned long long) mOverlay );

   decode_uintX(in, 8, mTtl);
   StackLog( << "mTtl =" << std::hex << (unsigned long long) mTtl );

   decode_uintX(in, 8, mReserved);
   StackLog( << "mReserved =" << std::hex << (unsigned long long) mReserved );

   decode_uintX(in, 16, mFragment);
   StackLog( << "mFragment =" << std::hex << (unsigned long long) mFragment );

   decode_uintX(in, 8, mVersion);
   StackLog( << "mVersion =" << std::hex << (unsigned long long) mVersion );

   decode_uintX(in, 24, mLength);
   StackLog( << "mLength =" << std::hex << (unsigned long long) mLength );

   decode_uintX(in, 64, mTransactionId);
   StackLog( << "mTransactionId =" << std::hex << (unsigned long long) mTransactionId );

   decode_uintX(in, 16, mFlags);
   StackLog( << "mFlags =" << std::hex << (unsigned long long) mFlags );

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mViaList.push_back(0);
      mViaList[i] = new DestinationStruct();
   mViaList[i]->decode(in2);
      i++;
   }
;   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mDestinationList.push_back(0);
      mDestinationList[i] = new DestinationStruct();
   mDestinationList[i]->decode(in2);
      i++;
   }
;   }

   decode_uintX(in, 16, mRouteLogLenDummy);
   StackLog( << "mRouteLogLenDummy =" << std::hex << (unsigned long long) mRouteLogLenDummy );

   decode_uintX(in, 16, mMessageCode);
   StackLog( << "mMessageCode =" << std::hex << (unsigned long long) mMessageCode );

};

void ForwardingHeaderStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ForwardingHeaderStruct");
   StackLog( << "mReloToken =" << std::hex << (unsigned long long) mReloToken );
   encode_uintX(out, 32, mReloToken);

   StackLog( << "mOverlay =" << std::hex << (unsigned long long) mOverlay );
   encode_uintX(out, 32, mOverlay);

   StackLog( << "mTtl =" << std::hex << (unsigned long long) mTtl );
   encode_uintX(out, 8, mTtl);

   StackLog( << "mReserved =" << std::hex << (unsigned long long) mReserved );
   encode_uintX(out, 8, mReserved);

   StackLog( << "mFragment =" << std::hex << (unsigned long long) mFragment );
   encode_uintX(out, 16, mFragment);

   StackLog( << "mVersion =" << std::hex << (unsigned long long) mVersion );
   encode_uintX(out, 8, mVersion);

   StackLog( << "mLength =" << std::hex << (unsigned long long) mLength );
   encode_uintX(out, 24, mLength);

   StackLog( << "mTransactionId =" << std::hex << (unsigned long long) mTransactionId );
   encode_uintX(out, 64, mTransactionId);

   StackLog( << "mFlags =" << std::hex << (unsigned long long) mFlags );
   encode_uintX(out, 16, mFlags);

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mViaList.size();i++)
   {
      mViaList[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mDestinationList.size();i++)
   {
      mDestinationList[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

   StackLog( << "mRouteLogLenDummy =" << std::hex << (unsigned long long) mRouteLogLenDummy );
   encode_uintX(out, 16, mRouteLogLenDummy);

   StackLog( << "mMessageCode =" << std::hex << (unsigned long long) mMessageCode );
   encode_uintX(out, 16, mMessageCode);

};



// Classes for ForwardingLayerMessageStruct */

ForwardingLayerMessageStruct :: ForwardingLayerMessageStruct ()
{
   mName = "ForwardingLayerMessageStruct";
 StackLog(<< "Constructing ForwardingLayerMessageStruct");
   mHeader=0;


   mSig=0;

};

ForwardingLayerMessageStruct :: ForwardingLayerMessageStruct (const ForwardingLayerMessageStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ForwardingLayerMessageStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void ForwardingLayerMessageStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ForwardingLayerMessage:\n";
   indent+=2;
   mHeader->print(out, indent);
    out << mPayload.hex();
   mSig->print(out, indent);
};

void ForwardingLayerMessageStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding ForwardingLayerMessageStruct");
   mHeader = new ForwardingHeaderStruct();
   mHeader->decode(in);

   {
      UInt32 len;
      int c;
      decode_uintX(in, 24, len);
      resip::DataStream strm(mPayload);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "payload",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   mSig = new SignatureStruct();
   mSig->decode(in);

};

void ForwardingLayerMessageStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ForwardingLayerMessageStruct");
   mHeader->encode(out);

    encode_uintX(out, 24, mPayload.size());
    out << mPayload;

   mSig->encode(out);

};



// Classes for MessagePayloadStruct */

MessagePayloadStruct :: MessagePayloadStruct ()
{
   mName = "MessagePayloadStruct";
 StackLog(<< "Constructing MessagePayloadStruct");

};

MessagePayloadStruct :: MessagePayloadStruct (const MessagePayloadStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "MessagePayloadStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void MessagePayloadStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "MessagePayload:\n";
   indent+=2;
    out << mPayload.hex();
};

void MessagePayloadStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding MessagePayloadStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 24, len);
      resip::DataStream strm(mPayload);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "payload",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void MessagePayloadStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding MessagePayloadStruct");
    encode_uintX(out, 24, mPayload.size());
    out << mPayload;

};



// Classes for ErrorResponseStruct */

ErrorResponseStruct :: ErrorResponseStruct ()
{
   mName = "ErrorResponseStruct";
 StackLog(<< "Constructing ErrorResponseStruct");
   mErrorCode=0;



};

ErrorResponseStruct :: ErrorResponseStruct (const ErrorResponseStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ErrorResponseStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void ErrorResponseStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ErrorResponse:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "error_code:" << std::hex << (unsigned long long)mErrorCode << "\n"; 
    out << mReasonPhrase.hex();
    out << mErrorInfo.hex();
};

void ErrorResponseStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding ErrorResponseStruct");
   decode_uintX(in, 16, mErrorCode);
   StackLog( << "mErrorCode =" << std::hex << (unsigned long long) mErrorCode );

   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mReasonPhrase);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "reason_phrase",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mErrorInfo);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "error_info",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void ErrorResponseStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ErrorResponseStruct");
   StackLog( << "mErrorCode =" << std::hex << (unsigned long long) mErrorCode );
   encode_uintX(out, 16, mErrorCode);

    encode_uintX(out, 8, mReasonPhrase.size());
    out << mReasonPhrase;

    encode_uintX(out, 16, mErrorInfo.size());
    out << mErrorInfo;

};



// Classes for JoinReqStruct */

JoinReqStruct :: JoinReqStruct ()
{
   mName = "JoinReqStruct";
 StackLog(<< "Constructing JoinReqStruct");
   mJoiningPeerId=0;


};

JoinReqStruct :: JoinReqStruct (const JoinReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "JoinReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void JoinReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "JoinReq:\n";
   indent+=2;
   mJoiningPeerId->print(out, indent);
    out << mOverlaySpecificData.hex();
};

void JoinReqStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding JoinReqStruct");
   mJoiningPeerId = new NodeIdStruct();
   mJoiningPeerId->decode(in);

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mOverlaySpecificData);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "overlay_specific_data",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void JoinReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding JoinReqStruct");
   mJoiningPeerId->encode(out);

    encode_uintX(out, 16, mOverlaySpecificData.size());
    out << mOverlaySpecificData;

};



// Classes for JoinAnsStruct */

JoinAnsStruct :: JoinAnsStruct ()
{
   mName = "JoinAnsStruct";
 StackLog(<< "Constructing JoinAnsStruct");

};

JoinAnsStruct :: JoinAnsStruct (const JoinAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "JoinAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void JoinAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "JoinAns:\n";
   indent+=2;
    out << mOverlaySpecificData.hex();
};

void JoinAnsStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding JoinAnsStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mOverlaySpecificData);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "overlay_specific_data",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void JoinAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding JoinAnsStruct");
    encode_uintX(out, 16, mOverlaySpecificData.size());
    out << mOverlaySpecificData;

};



// Classes for LeaveReqStruct */

LeaveReqStruct :: LeaveReqStruct ()
{
   mName = "LeaveReqStruct";
 StackLog(<< "Constructing LeaveReqStruct");
   mLeavingPeerId=0;


};

LeaveReqStruct :: LeaveReqStruct (const LeaveReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "LeaveReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void LeaveReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "LeaveReq:\n";
   indent+=2;
   mLeavingPeerId->print(out, indent);
    out << mOverlaySpecificData.hex();
};

void LeaveReqStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding LeaveReqStruct");
   mLeavingPeerId = new NodeIdStruct();
   mLeavingPeerId->decode(in);

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mOverlaySpecificData);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "overlay_specific_data",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void LeaveReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding LeaveReqStruct");
   mLeavingPeerId->encode(out);

    encode_uintX(out, 16, mOverlaySpecificData.size());
    out << mOverlaySpecificData;

};



// Classes for RouteQueryReqStruct */

RouteQueryReqStruct :: RouteQueryReqStruct ()
{
   mName = "RouteQueryReqStruct";
 StackLog(<< "Constructing RouteQueryReqStruct");
   mSendUpdate=(Boolean)0;

   mDestination=0;


};

RouteQueryReqStruct :: RouteQueryReqStruct (const RouteQueryReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "RouteQueryReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void RouteQueryReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "RouteQueryReq:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "send_update:" << std::hex << (unsigned long long) mSendUpdate << "\n"; 
   mDestination->print(out, indent);
    out << mOverlaySpecificData.hex();
};

void RouteQueryReqStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding RouteQueryReqStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mSendUpdate=(Boolean)v;
   }

   mDestination = new DestinationStruct();
   mDestination->decode(in);

   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mOverlaySpecificData);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "overlay_specific_data",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void RouteQueryReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding RouteQueryReqStruct");
   encode_uintX(out, 8, (u_int64)(mSendUpdate));

   mDestination->encode(out);

    encode_uintX(out, 16, mOverlaySpecificData.size());
    out << mOverlaySpecificData;

};



// Classes for FramedMessageStruct */

FramedMessageStruct :: FramedMessageStruct ()
{
   mName = "FramedMessageStruct";
 StackLog(<< "Constructing FramedMessageStruct");
   mType=(FramedMessageType)0;

   mData.mSequence=0;
   mAck.mAckSequence=0;
   mAck.mReceived=0;

};

FramedMessageStruct :: FramedMessageStruct (const FramedMessageStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FramedMessageStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FramedMessageStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(FramedMessageType)v;
   }

   switch(mType){
      case 128:
            decode_uintX(in, 24, mData.mSequence);
   StackLog( << "mData.mSequence =" << std::hex << (unsigned long long) mData.mSequence );
            {
      UInt32 len;
      int c;
      decode_uintX(in, 24, len);
      resip::DataStream strm(mData.mMessage);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "message",__FILE__,__LINE__);
          strm.put(c);
      };
   }
          break;

      case 129:
            decode_uintX(in, 24, mAck.mAckSequence);
   StackLog( << "mAck.mAckSequence =" << std::hex << (unsigned long long) mAck.mAckSequence );
            decode_uintX(in, 32, mAck.mReceived);
   StackLog( << "mAck.mReceived =" << std::hex << (unsigned long long) mAck.mReceived );
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};

void FramedMessageStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FramedMessageStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 128:
            StackLog( << "mData.mSequence =" << std::hex << (unsigned long long) mData.mSequence );
   encode_uintX(out, 24, mData.mSequence);
             encode_uintX(out, 24, mData.mMessage.size());
    out << mData.mMessage;
          break;

      case 129:
            StackLog( << "mAck.mAckSequence =" << std::hex << (unsigned long long) mAck.mAckSequence );
   encode_uintX(out, 24, mAck.mAckSequence);
            StackLog( << "mAck.mReceived =" << std::hex << (unsigned long long) mAck.mReceived );
   encode_uintX(out, 32, mAck.mReceived);
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};



// Classes for IceCandidateStruct */

IceCandidateStruct :: IceCandidateStruct ()
{
   mName = "IceCandidateStruct";
 StackLog(<< "Constructing IceCandidateStruct");

};

IceCandidateStruct :: IceCandidateStruct (const IceCandidateStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "IceCandidateStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void IceCandidateStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "IceCandidate:\n";
   indent+=2;
    out << mCandidate.hex();
};

void IceCandidateStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding IceCandidateStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mCandidate);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "candidate",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void IceCandidateStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding IceCandidateStruct");
    encode_uintX(out, 16, mCandidate.size());
    out << mCandidate;

};



// Classes for ConnectReqAnsStruct */

ConnectReqAnsStruct :: ConnectReqAnsStruct ()
{
   mName = "ConnectReqAnsStruct";
 StackLog(<< "Constructing ConnectReqAnsStruct");


   mApplication=0;



};

ConnectReqAnsStruct :: ConnectReqAnsStruct (const ConnectReqAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ConnectReqAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void ConnectReqAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ConnectReqAns:\n";
   indent+=2;
    out << mUfrag.hex();
    out << mPassword.hex();
   do_indent(out, indent);
   (out)  << "application:" << std::hex << (unsigned long long)mApplication << "\n"; 
    out << mRole.hex();
   for(unsigned int i=0;i<mCandidates.size();i++){
      mCandidates[i]->print(out, indent);
   }
};

void ConnectReqAnsStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding ConnectReqAnsStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mUfrag);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "ufrag",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mPassword);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "password",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   decode_uintX(in, 16, mApplication);
   StackLog( << "mApplication =" << std::hex << (unsigned long long) mApplication );

   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mRole);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "role",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mCandidates.push_back(0);
      mCandidates[i] = new IceCandidateStruct();
   mCandidates[i]->decode(in2);
      i++;
   }
;   }

};

void ConnectReqAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ConnectReqAnsStruct");
    encode_uintX(out, 8, mUfrag.size());
    out << mUfrag;

    encode_uintX(out, 8, mPassword.size());
    out << mPassword;

   StackLog( << "mApplication =" << std::hex << (unsigned long long) mApplication );
   encode_uintX(out, 16, mApplication);

    encode_uintX(out, 8, mRole.size());
    out << mRole;

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mCandidates.size();i++)
   {
      mCandidates[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for PingReqStruct */

PingReqStruct :: PingReqStruct ()
{
   mName = "PingReqStruct";
 StackLog(<< "Constructing PingReqStruct");

};

PingReqStruct :: PingReqStruct (const PingReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "PingReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding PingReqStruct");
   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mRequestedInfo.push_back(0);
      decode_uintX(in2, 8, mRequestedInfo[i]);
   StackLog( << "mRequestedInfo[i] =" << std::hex << (unsigned long long) mRequestedInfo[i] );
      i++;
   }
;   }

};

void PingReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding PingReqStruct");
   {
   long pos1=TELLP(out);
   for(int i=0;i<1;i++) out.put(0);
   for(unsigned int i=0;i<mRequestedInfo.size();i++)
   {
      StackLog( << "mRequestedInfo[i] =" << std::hex << (unsigned long long) mRequestedInfo[i] );
   encode_uintX(out, 8, mRequestedInfo[i]);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   SEEKP(out,pos2);
   }

};



// Classes for PingInformationStruct */

PingInformationStruct :: PingInformationStruct ()
{
   mName = "PingInformationStruct";
 StackLog(<< "Constructing PingInformationStruct");
   mType=(PingInformationType)0;

   mResponsibleSet.mResponsiblePpb=0;
   mNumResources.mNumResources=0;

};

PingInformationStruct :: PingInformationStruct (const PingInformationStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "PingInformationStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding PingInformationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(PingInformationType)v;
   }

   switch(mType){
      case 1:
            decode_uintX(in, 32, mResponsibleSet.mResponsiblePpb);
   StackLog( << "mResponsibleSet.mResponsiblePpb =" << std::hex << (unsigned long long) mResponsibleSet.mResponsiblePpb );
          break;

      case 2:
            decode_uintX(in, 32, mNumResources.mNumResources);
   StackLog( << "mNumResources.mNumResources =" << std::hex << (unsigned long long) mNumResources.mNumResources );
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};

void PingInformationStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding PingInformationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 1:
            StackLog( << "mResponsibleSet.mResponsiblePpb =" << std::hex << (unsigned long long) mResponsibleSet.mResponsiblePpb );
   encode_uintX(out, 32, mResponsibleSet.mResponsiblePpb);
          break;

      case 2:
            StackLog( << "mNumResources.mNumResources =" << std::hex << (unsigned long long) mNumResources.mNumResources );
   encode_uintX(out, 32, mNumResources.mNumResources);
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};



// Classes for PingAnsStruct */

PingAnsStruct :: PingAnsStruct ()
{
   mName = "PingAnsStruct";
 StackLog(<< "Constructing PingAnsStruct");
   mResponseId=0;


};

PingAnsStruct :: PingAnsStruct (const PingAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "PingAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding PingAnsStruct");
   decode_uintX(in, 64, mResponseId);
   StackLog( << "mResponseId =" << std::hex << (unsigned long long) mResponseId );

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mPingInfo.push_back(0);
      mPingInfo[i] = new PingInformationStruct();
   mPingInfo[i]->decode(in2);
      i++;
   }
;   }

};

void PingAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding PingAnsStruct");
   StackLog( << "mResponseId =" << std::hex << (unsigned long long) mResponseId );
   encode_uintX(out, 64, mResponseId);

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mPingInfo.size();i++)
   {
      mPingInfo[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for TunnelReqStruct */

TunnelReqStruct :: TunnelReqStruct ()
{
   mName = "TunnelReqStruct";
 StackLog(<< "Constructing TunnelReqStruct");
   mApplication=0;



};

TunnelReqStruct :: TunnelReqStruct (const TunnelReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "TunnelReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void TunnelReqStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "TunnelReq:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "application:" << std::hex << (unsigned long long)mApplication << "\n"; 
    out << mDialogId.hex();
    out << mApplicationPdu.hex();
};

void TunnelReqStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding TunnelReqStruct");
   decode_uintX(in, 16, mApplication);
   StackLog( << "mApplication =" << std::hex << (unsigned long long) mApplication );

   {
      UInt32 len;
      int c;
      decode_uintX(in, 8, len);
      resip::DataStream strm(mDialogId);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "dialog_id",__FILE__,__LINE__);
          strm.put(c);
      };
   }

   {
      UInt32 len;
      int c;
      decode_uintX(in, 24, len);
      resip::DataStream strm(mApplicationPdu);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "application_pdu",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void TunnelReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding TunnelReqStruct");
   StackLog( << "mApplication =" << std::hex << (unsigned long long) mApplication );
   encode_uintX(out, 16, mApplication);

    encode_uintX(out, 8, mDialogId.size());
    out << mDialogId;

    encode_uintX(out, 24, mApplicationPdu.size());
    out << mApplicationPdu;

};



// Classes for DataValueStruct */

DataValueStruct :: DataValueStruct ()
{
   mName = "DataValueStruct";
 StackLog(<< "Constructing DataValueStruct");
   mExists=(Boolean)0;


};

DataValueStruct :: DataValueStruct (const DataValueStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "DataValueStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void DataValueStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "DataValue:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "exists:" << std::hex << (unsigned long long) mExists << "\n"; 
    out << mValue.hex();
};

void DataValueStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding DataValueStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mExists=(Boolean)v;
   }

   {
      UInt32 len;
      int c;
      decode_uintX(in, 32, len);
      resip::DataStream strm(mValue);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "value",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void DataValueStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding DataValueStruct");
   encode_uintX(out, 8, (u_int64)(mExists));

    encode_uintX(out, 32, mValue.size());
    out << mValue;

};



// Classes for ArrayEntryStruct */

ArrayEntryStruct :: ArrayEntryStruct ()
{
   mName = "ArrayEntryStruct";
 StackLog(<< "Constructing ArrayEntryStruct");
   mIndex=0;

   mValue=0;

};

ArrayEntryStruct :: ArrayEntryStruct (const ArrayEntryStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ArrayEntryStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding ArrayEntryStruct");
   decode_uintX(in, 32, mIndex);
   StackLog( << "mIndex =" << std::hex << (unsigned long long) mIndex );

   mValue = new DataValueStruct();
   mValue->decode(in);

};

void ArrayEntryStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ArrayEntryStruct");
   StackLog( << "mIndex =" << std::hex << (unsigned long long) mIndex );
   encode_uintX(out, 32, mIndex);

   mValue->encode(out);

};



// Classes for DictionaryKeyStruct */

DictionaryKeyStruct :: DictionaryKeyStruct ()
{
   mName = "DictionaryKeyStruct";
 StackLog(<< "Constructing DictionaryKeyStruct");

};

DictionaryKeyStruct :: DictionaryKeyStruct (const DictionaryKeyStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "DictionaryKeyStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void DictionaryKeyStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "DictionaryKey:\n";
   indent+=2;
    out << mKey.hex();
};

void DictionaryKeyStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding DictionaryKeyStruct");
   {
      UInt32 len;
      int c;
      decode_uintX(in, 16, len);
      resip::DataStream strm(mKey);
      while(len--){
        c=in.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "key",__FILE__,__LINE__);
          strm.put(c);
      };
   }

};

void DictionaryKeyStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding DictionaryKeyStruct");
    encode_uintX(out, 16, mKey.size());
    out << mKey;

};



// Classes for DictionaryEntryStruct */

DictionaryEntryStruct :: DictionaryEntryStruct ()
{
   mName = "DictionaryEntryStruct";
 StackLog(<< "Constructing DictionaryEntryStruct");
   mKey=0;

   mValue=0;

};

DictionaryEntryStruct :: DictionaryEntryStruct (const DictionaryEntryStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "DictionaryEntryStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding DictionaryEntryStruct");
   mKey = new DictionaryKeyStruct();
   mKey->decode(in);

   mValue = new DataValueStruct();
   mValue->decode(in);

};

void DictionaryEntryStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding DictionaryEntryStruct");
   mKey->encode(out);

   mValue->encode(out);

};



// Classes for StoredDataValueStruct */

StoredDataValueStruct :: StoredDataValueStruct ()
{
   mName = "StoredDataValueStruct";
 StackLog(<< "Constructing StoredDataValueStruct");
   mModel=(DataModel)0;

   mSingleValue.mSingleValueEntry=0;
   mArray.mArrayEntry=0;
   mDictionary.mDictionaryEntry=0;

};

StoredDataValueStruct :: StoredDataValueStruct (const StoredDataValueStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoredDataValueStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding StoredDataValueStruct");
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
          resip_assert(1==0);
   }


};

void StoredDataValueStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoredDataValueStruct");
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
          resip_assert(1==0);
   }


};



// Classes for StoredDataStruct */

StoredDataStruct :: StoredDataStruct ()
{
   mName = "StoredDataStruct";
 StackLog(<< "Constructing StoredDataStruct");

   mStorageTime=0;

   mLifetime=0;

   mValue=0;

   mSignature=0;

};

StoredDataStruct :: StoredDataStruct (const StoredDataStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoredDataStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void StoredDataStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "StoredData:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "storage_time:" << std::hex << (unsigned long long)mStorageTime << "\n"; 
   do_indent(out, indent);
   (out)  << "lifetime:" << std::hex << (unsigned long long)mLifetime << "\n"; 
   mValue->print(out, indent);
   mSignature->print(out, indent);
};

void StoredDataStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding StoredDataStruct");
   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in_auto(d);

   decode_uintX(in_auto, 64, mStorageTime);
   StackLog( << "mStorageTime =" << std::hex << (unsigned long long) mStorageTime );

   decode_uintX(in_auto, 32, mLifetime);
   StackLog( << "mLifetime =" << std::hex << (unsigned long long) mLifetime );

   mValue = new StoredDataValueStruct();
   mValue->decode(in_auto);

   mSignature = new SignatureStruct();
   mSignature->decode(in_auto);

   if(in_auto.peek()!=EOF)
      throw resip::ParseException("Inner encoded value too long",
      "StoredData",__FILE__,__LINE__);
   }
};

void StoredDataStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoredDataStruct");
   long pos1=TELLP(out);
   for(int i=0;i<4;i++) out.put(0);

   StackLog( << "mStorageTime =" << std::hex << (unsigned long long) mStorageTime );
   encode_uintX(out, 64, mStorageTime);

   StackLog( << "mLifetime =" << std::hex << (unsigned long long) mLifetime );
   encode_uintX(out, 32, mLifetime);

   mValue->encode(out);

   mSignature->encode(out);

   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   SEEKP(out,pos2);
};



// Classes for StoreKindDataStruct */

StoreKindDataStruct :: StoreKindDataStruct ()
{
   mName = "StoreKindDataStruct";
 StackLog(<< "Constructing StoreKindDataStruct");
   mKind=0;

   mDataModel=(DataModel)0;

   mGenerationCounter=0;


};

StoreKindDataStruct :: StoreKindDataStruct (const StoreKindDataStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoreKindDataStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding StoreKindDataStruct");
   decode_uintX(in, 32, mKind);
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );

   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mDataModel=(DataModel)v;
   }

   decode_uintX(in, 64, mGenerationCounter);
   StackLog( << "mGenerationCounter =" << std::hex << (unsigned long long) mGenerationCounter );

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mValues.push_back(0);
      mValues[i] = new StoredDataStruct();
   mValues[i]->decode(in2);
      i++;
   }
;   }

};

void StoreKindDataStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoreKindDataStruct");
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 8, (u_int64)(mDataModel));

   StackLog( << "mGenerationCounter =" << std::hex << (unsigned long long) mGenerationCounter );
   encode_uintX(out, 64, mGenerationCounter);

   {
   long pos1=TELLP(out);
   for(int i=0;i<4;i++) out.put(0);
   for(unsigned int i=0;i<mValues.size();i++)
   {
      mValues[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   SEEKP(out,pos2);
   }

};



// Classes for StoreReqStruct */

StoreReqStruct :: StoreReqStruct ()
{
   mName = "StoreReqStruct";
 StackLog(<< "Constructing StoreReqStruct");
   mResource=0;

   mReplicaNumber=0;


};

StoreReqStruct :: StoreReqStruct (const StoreReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoreReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding StoreReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   decode_uintX(in, 8, mReplicaNumber);
   StackLog( << "mReplicaNumber =" << std::hex << (unsigned long long) mReplicaNumber );

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindData.push_back(0);
      mKindData[i] = new StoreKindDataStruct();
   mKindData[i]->decode(in2);
      i++;
   }
;   }

};

void StoreReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoreReqStruct");
   mResource->encode(out);

   StackLog( << "mReplicaNumber =" << std::hex << (unsigned long long) mReplicaNumber );
   encode_uintX(out, 8, mReplicaNumber);

   {
   long pos1=TELLP(out);
   for(int i=0;i<4;i++) out.put(0);
   for(unsigned int i=0;i<mKindData.size();i++)
   {
      mKindData[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   SEEKP(out,pos2);
   }

};



// Classes for StoreKindResponseStruct */

StoreKindResponseStruct :: StoreKindResponseStruct ()
{
   mName = "StoreKindResponseStruct";
 StackLog(<< "Constructing StoreKindResponseStruct");
   mKind=0;

   mGenerationCounter=0;


};

StoreKindResponseStruct :: StoreKindResponseStruct (const StoreKindResponseStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoreKindResponseStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding StoreKindResponseStruct");
   decode_uintX(in, 32, mKind);
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );

   decode_uintX(in, 64, mGenerationCounter);
   StackLog( << "mGenerationCounter =" << std::hex << (unsigned long long) mGenerationCounter );

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mReplicas.push_back(0);
      mReplicas[i] = new NodeIdStruct();
   mReplicas[i]->decode(in2);
      i++;
   }
;   }

};

void StoreKindResponseStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoreKindResponseStruct");
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );
   encode_uintX(out, 32, mKind);

   StackLog( << "mGenerationCounter =" << std::hex << (unsigned long long) mGenerationCounter );
   encode_uintX(out, 64, mGenerationCounter);

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mReplicas.size();i++)
   {
      mReplicas[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for StoreAnsStruct */

StoreAnsStruct :: StoreAnsStruct ()
{
   mName = "StoreAnsStruct";
 StackLog(<< "Constructing StoreAnsStruct");

};

StoreAnsStruct :: StoreAnsStruct (const StoreAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoreAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding StoreAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i] = new StoreKindResponseStruct();
   mKindResponses[i]->decode(in2);
      i++;
   }
;   }

};

void StoreAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoreAnsStruct");
   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mKindResponses.size();i++)
   {
      mKindResponses[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for ArrayRangeStruct */

ArrayRangeStruct :: ArrayRangeStruct ()
{
   mName = "ArrayRangeStruct";
 StackLog(<< "Constructing ArrayRangeStruct");
   mFirst=0;

   mLast=0;

};

ArrayRangeStruct :: ArrayRangeStruct (const ArrayRangeStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ArrayRangeStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding ArrayRangeStruct");
   decode_uintX(in, 32, mFirst);
   StackLog( << "mFirst =" << std::hex << (unsigned long long) mFirst );

   decode_uintX(in, 32, mLast);
   StackLog( << "mLast =" << std::hex << (unsigned long long) mLast );

};

void ArrayRangeStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ArrayRangeStruct");
   StackLog( << "mFirst =" << std::hex << (unsigned long long) mFirst );
   encode_uintX(out, 32, mFirst);

   StackLog( << "mLast =" << std::hex << (unsigned long long) mLast );
   encode_uintX(out, 32, mLast);

};



// Classes for StoredDataSpecifierStruct */

StoredDataSpecifierStruct :: StoredDataSpecifierStruct ()
{
   mName = "StoredDataSpecifierStruct";
 StackLog(<< "Constructing StoredDataSpecifierStruct");
   mKind=0;

   mModel=(DataModel)0;

   mGeneration=0;



};

StoredDataSpecifierStruct :: StoredDataSpecifierStruct (const StoredDataSpecifierStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "StoredDataSpecifierStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
};

void StoredDataSpecifierStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding StoredDataSpecifierStruct");
   decode_uintX(in, 32, mKind);
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );

   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mModel=(DataModel)v;
   }

   decode_uintX(in, 64, mGeneration);
   StackLog( << "mGeneration =" << std::hex << (unsigned long long) mGeneration );

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in_auto(d);

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
               mArray.mIndices[i] = new ArrayRangeStruct();
   mArray.mIndices[i]->decode(in2);
      i++;
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
               mDictionary.mKeys[i] = new DictionaryKeyStruct();
   mDictionary.mKeys[i]->decode(in2);
      i++;
   }
;   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   if(in_auto.peek()!=EOF)
      throw resip::ParseException("Inner encoded value too long",
      "StoredDataSpecifier",__FILE__,__LINE__);
   }
};

void StoredDataSpecifierStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding StoredDataSpecifierStruct");
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );
   encode_uintX(out, 32, mKind);

   encode_uintX(out, 8, (u_int64)(mModel));

   StackLog( << "mGeneration =" << std::hex << (unsigned long long) mGeneration );
   encode_uintX(out, 64, mGeneration);

   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);

   switch(mModel) {
      case 1:
          break;

      case 2:
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mArray.mIndices.size();i++)
   {
               mArray.mIndices[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
          break;

      case 3:
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mDictionary.mKeys.size();i++)
   {
               mDictionary.mKeys[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
};



// Classes for FetchReqStruct */

FetchReqStruct :: FetchReqStruct ()
{
   mName = "FetchReqStruct";
 StackLog(<< "Constructing FetchReqStruct");
   mResource=0;


};

FetchReqStruct :: FetchReqStruct (const FetchReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FetchReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FetchReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSpecifiers.push_back(0);
      mSpecifiers[i] = new StoredDataSpecifierStruct();
   mSpecifiers[i]->decode(in2);
      i++;
   }
;   }

};

void FetchReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FetchReqStruct");
   mResource->encode(out);

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mSpecifiers.size();i++)
   {
      mSpecifiers[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for FetchKindResponseStruct */

FetchKindResponseStruct :: FetchKindResponseStruct ()
{
   mName = "FetchKindResponseStruct";
 StackLog(<< "Constructing FetchKindResponseStruct");
   mKind=0;

   mGeneration=0;


};

FetchKindResponseStruct :: FetchKindResponseStruct (const FetchKindResponseStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FetchKindResponseStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FetchKindResponseStruct");
   decode_uintX(in, 32, mKind);
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );

   decode_uintX(in, 64, mGeneration);
   StackLog( << "mGeneration =" << std::hex << (unsigned long long) mGeneration );

   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mValues.push_back(0);
      mValues[i] = new StoredDataStruct();
   mValues[i]->decode(in2);
      i++;
   }
;   }

};

void FetchKindResponseStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FetchKindResponseStruct");
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );
   encode_uintX(out, 32, mKind);

   StackLog( << "mGeneration =" << std::hex << (unsigned long long) mGeneration );
   encode_uintX(out, 64, mGeneration);

   {
   long pos1=TELLP(out);
   for(int i=0;i<4;i++) out.put(0);
   for(unsigned int i=0;i<mValues.size();i++)
   {
      mValues[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   SEEKP(out,pos2);
   }

};



// Classes for FetchAnsStruct */

FetchAnsStruct :: FetchAnsStruct ()
{
   mName = "FetchAnsStruct";
 StackLog(<< "Constructing FetchAnsStruct");

};

FetchAnsStruct :: FetchAnsStruct (const FetchAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FetchAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FetchAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 4, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i] = new FetchKindResponseStruct();
   mKindResponses[i]->decode(in2);
      i++;
   }
;   }

};

void FetchAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FetchAnsStruct");
   {
   long pos1=TELLP(out);
   for(int i=0;i<4;i++) out.put(0);
   for(unsigned int i=0;i<mKindResponses.size();i++)
   {
      mKindResponses[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 32, (pos2 - pos1) - 4);
   SEEKP(out,pos2);
   }

};



// Classes for RemoveReqStruct */

RemoveReqStruct :: RemoveReqStruct ()
{
   mName = "RemoveReqStruct";
 StackLog(<< "Constructing RemoveReqStruct");
   mResource=0;


};

RemoveReqStruct :: RemoveReqStruct (const RemoveReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "RemoveReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding RemoveReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSpecifiers.push_back(0);
      mSpecifiers[i] = new StoredDataSpecifierStruct();
   mSpecifiers[i]->decode(in2);
      i++;
   }
;   }

};

void RemoveReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding RemoveReqStruct");
   mResource->encode(out);

   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mSpecifiers.size();i++)
   {
      mSpecifiers[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for RemoveAnsStruct */

RemoveAnsStruct :: RemoveAnsStruct ()
{
   mName = "RemoveAnsStruct";
 StackLog(<< "Constructing RemoveAnsStruct");

};

RemoveAnsStruct :: RemoveAnsStruct (const RemoveAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "RemoveAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding RemoveAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKindResponses.push_back(0);
      mKindResponses[i] = new StoreKindResponseStruct();
   mKindResponses[i]->decode(in2);
      i++;
   }
;   }

};

void RemoveAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding RemoveAnsStruct");
   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mKindResponses.size();i++)
   {
      mKindResponses[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for FindReqStruct */

FindReqStruct :: FindReqStruct ()
{
   mName = "FindReqStruct";
 StackLog(<< "Constructing FindReqStruct");
   mResource=0;


};

FindReqStruct :: FindReqStruct (const FindReqStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FindReqStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FindReqStruct");
   mResource = new ResourceIdStruct();
   mResource->decode(in);

   {
   resip::Data d;
   read_varray1(in, 1, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mKinds.push_back(0);
      decode_uintX(in2, 32, mKinds[i]);
   StackLog( << "mKinds[i] =" << std::hex << (unsigned long long) mKinds[i] );
      i++;
   }
;   }

};

void FindReqStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FindReqStruct");
   mResource->encode(out);

   {
   long pos1=TELLP(out);
   for(int i=0;i<1;i++) out.put(0);
   for(unsigned int i=0;i<mKinds.size();i++)
   {
      StackLog( << "mKinds[i] =" << std::hex << (unsigned long long) mKinds[i] );
   encode_uintX(out, 32, mKinds[i]);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 8, (pos2 - pos1) - 1);
   SEEKP(out,pos2);
   }

};



// Classes for FindKindDataStruct */

FindKindDataStruct :: FindKindDataStruct ()
{
   mName = "FindKindDataStruct";
 StackLog(<< "Constructing FindKindDataStruct");
   mKind=0;

   mClosest=0;

};

FindKindDataStruct :: FindKindDataStruct (const FindKindDataStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FindKindDataStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FindKindDataStruct");
   decode_uintX(in, 32, mKind);
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );

   mClosest = new ResourceIdStruct();
   mClosest->decode(in);

};

void FindKindDataStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FindKindDataStruct");
   StackLog( << "mKind =" << std::hex << (unsigned long long) mKind );
   encode_uintX(out, 32, mKind);

   mClosest->encode(out);

};



// Classes for FindAnsStruct */

FindAnsStruct :: FindAnsStruct ()
{
   mName = "FindAnsStruct";
 StackLog(<< "Constructing FindAnsStruct");

};

FindAnsStruct :: FindAnsStruct (const FindAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "FindAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding FindAnsStruct");
   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mResults.push_back(0);
      mResults[i] = new FindKindDataStruct();
   mResults[i]->decode(in2);
      i++;
   }
;   }

};

void FindAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding FindAnsStruct");
   {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mResults.size();i++)
   {
      mResults[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }

};



// Classes for TurnServerStruct */

TurnServerStruct :: TurnServerStruct ()
{
   mName = "TurnServerStruct";
 StackLog(<< "Constructing TurnServerStruct");
   mIteration=0;

   mServerAddress=0;

};

TurnServerStruct :: TurnServerStruct (const TurnServerStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "TurnServerStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding TurnServerStruct");
   decode_uintX(in, 8, mIteration);
   StackLog( << "mIteration =" << std::hex << (unsigned long long) mIteration );

   mServerAddress = new IpAddressAndPortStruct();
   mServerAddress->decode(in);

};

void TurnServerStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding TurnServerStruct");
   StackLog( << "mIteration =" << std::hex << (unsigned long long) mIteration );
   encode_uintX(out, 8, mIteration);

   mServerAddress->encode(out);

};



// Classes for SipRegistrationStruct */

SipRegistrationStruct :: SipRegistrationStruct ()
{
   mName = "SipRegistrationStruct";
 StackLog(<< "Constructing SipRegistrationStruct");
   mType=(SipRegistrationType)0;



};

SipRegistrationStruct :: SipRegistrationStruct (const SipRegistrationStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "SipRegistrationStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void SipRegistrationStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "SipRegistration:\n";
   indent+=2;
   do_indent(out, indent);
   (out)  << "type:" << std::hex << (unsigned long long) mType << "\n"; 
};

void SipRegistrationStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding SipRegistrationStruct");
   {
      u_int32 v;
      decode_uintX(in, 8, v);
      mType=(SipRegistrationType)v;
   }

   {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in_auto(d);

   switch(mType){
      case 1:
            {
      UInt32 len;
      int c;
      decode_uintX(in_auto, 16, len);
      resip::DataStream strm(mSipRegistrationUri.mUri);
      while(len--){
        c=in_auto.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "uri",__FILE__,__LINE__);
          strm.put(c);
      };
   }
          break;

      case 2:
            {
      UInt32 len;
      int c;
      decode_uintX(in_auto, 16, len);
      resip::DataStream strm(mSipRegistrationRoute.mContactPrefs);
      while(len--){
        c=in_auto.get();
        if(c==EOF)
          throw resip::ParseException("Premature end of data",
          "contact_prefs",__FILE__,__LINE__);
          strm.put(c);
      };
   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mSipRegistrationRoute.mDestinationList.push_back(0);
               mSipRegistrationRoute.mDestinationList[i] = new DestinationStruct();
   mSipRegistrationRoute.mDestinationList[i]->decode(in2);
      i++;
   }
;   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   if(in_auto.peek()!=EOF)
      throw resip::ParseException("Inner encoded value too long",
      "SipRegistration",__FILE__,__LINE__);
   }
};

void SipRegistrationStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding SipRegistrationStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);

   switch(mType) {
      case 1:
             encode_uintX(out, 16, mSipRegistrationUri.mUri.size());
    out << mSipRegistrationUri.mUri;
          break;

      case 2:
             encode_uintX(out, 16, mSipRegistrationRoute.mContactPrefs.size());
    out << mSipRegistrationRoute.mContactPrefs;
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mSipRegistrationRoute.mDestinationList.size();i++)
   {
               mSipRegistrationRoute.mDestinationList[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
};



// Classes for ChordUpdateStruct */

ChordUpdateStruct :: ChordUpdateStruct ()
{
   mName = "ChordUpdateStruct";
 StackLog(<< "Constructing ChordUpdateStruct");
   mType=(ChordUpdateType)0;


};

ChordUpdateStruct :: ChordUpdateStruct (const ChordUpdateStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ChordUpdateStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

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
 StackLog(<< "Decoding ChordUpdateStruct");
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
               mNeighbors.mPredecessors[i] = new NodeIdStruct();
   mNeighbors.mPredecessors[i]->decode(in2);
      i++;
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mNeighbors.mSuccessors.push_back(0);
               mNeighbors.mSuccessors[i] = new NodeIdStruct();
   mNeighbors.mSuccessors[i]->decode(in2);
      i++;
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
               mFull.mPredecessors[i] = new NodeIdStruct();
   mFull.mPredecessors[i]->decode(in2);
      i++;
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mFull.mSuccessors.push_back(0);
               mFull.mSuccessors[i] = new NodeIdStruct();
   mFull.mSuccessors[i]->decode(in2);
      i++;
   }
;   }
            {
   resip::Data d;
   read_varray1(in, 2, d);
   resip::DataStream in2(d);
   int i=0;
   while(in2.peek()!=EOF){
      mFull.mFingers.push_back(0);
               mFull.mFingers[i] = new NodeIdStruct();
   mFull.mFingers[i]->decode(in2);
      i++;
   }
;   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};

void ChordUpdateStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ChordUpdateStruct");
   encode_uintX(out, 8, (u_int64)(mType));

   switch(mType) {
      case 1:
          break;

      case 2:
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mNeighbors.mPredecessors.size();i++)
   {
               mNeighbors.mPredecessors[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mNeighbors.mSuccessors.size();i++)
   {
               mNeighbors.mSuccessors[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
          break;

      case 3:
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mFull.mPredecessors.size();i++)
   {
               mFull.mPredecessors[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mFull.mSuccessors.size();i++)
   {
               mFull.mSuccessors[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
            {
   long pos1=TELLP(out);
   for(int i=0;i<2;i++) out.put(0);
   for(unsigned int i=0;i<mFull.mFingers.size();i++)
   {
               mFull.mFingers[i]->encode(out);
   }
   long pos2=TELLP(out);
   SEEKP(out,pos1);
   encode_uintX(out, 16, (pos2 - pos1) - 2);
   SEEKP(out,pos2);
   }
          break;

       default: /* User error */ 
          resip_assert(1==0);
   }


};



// Classes for ChordRouteQueryAnsStruct */

ChordRouteQueryAnsStruct :: ChordRouteQueryAnsStruct ()
{
   mName = "ChordRouteQueryAnsStruct";
 StackLog(<< "Constructing ChordRouteQueryAnsStruct");
   mNextId=0;

};

ChordRouteQueryAnsStruct :: ChordRouteQueryAnsStruct (const ChordRouteQueryAnsStruct &from)
{
   // World's lamest copy constructor
   if(this==&from) return;
   mName = "ChordRouteQueryAnsStruct";
   resip::Data dat;
   {
     resip::DataStream strm(dat);
     from.encode(strm);
   }
   {
     resip::DataStream strm(dat);
     decode(strm);
   }
}

void ChordRouteQueryAnsStruct :: print(std::ostream& out, int indent) const 
{
   do_indent(out,indent);
   (out) << "ChordRouteQueryAns:\n";
   indent+=2;
   mNextId->print(out, indent);
};

void ChordRouteQueryAnsStruct :: decode(std::istream& in)
{
 StackLog(<< "Decoding ChordRouteQueryAnsStruct");
   mNextId = new NodeIdStruct();
   mNextId->decode(in);

};

void ChordRouteQueryAnsStruct :: encode(std::ostream& out) const 
{
   StackLog(<< "Encoding ChordRouteQueryAnsStruct");
   mNextId->encode(out);

};

}
