#include "s2c_native.hxx"

namespace s2c {
namespace test {

class PeerIdPdu : public PDU {
public:
   unsigned char                 mId[16];


   PeerIdPdu() {mName = "peer_id";}
   PDUMemberFunctions
};


class SelectExamplePdu : public PDU {
public:
   int  mType;
   enum { 
          tAbc=5,
          tBcd=10
   };


   SelectExamplePdu() {mName = "select_example";}
   PDUMemberFunctions
};

class SelectExamplePdu__Abc : public SelectExamplePdu {
public:
   unsigned char                 mJjj;


   SelectExamplePdu__Abc() {mName = "abc";}
   PDUMemberFunctions
};


class SelectExamplePdu__Bcd : public SelectExamplePdu {
public:
   unsigned short                mWww;
   unsigned int                  mMmm;


   SelectExamplePdu__Bcd() {mName = "bcd";}
   PDUMemberFunctions
};


class BazTypePdu : public PDU {
public:
   unsigned short                mZzz;


   BazTypePdu() {mName = "baz_type";}
   PDUMemberFunctions
};


class FooPdu : public PDU {
public:
   unsigned char                 mBar;
   std::vector<unsigned char>    mVariable;
   BazTypePdu*                   mMumble;
   PeerIdPdu*                    mId;
   unsigned short                mZulu;
   unsigned char                 mWww[5];
   unsigned short                mWwx[3];


   FooPdu() {mName = "foo";}
   PDUMemberFunctions
};


}
}
