#include "s2c/s2c_native.hxx"

namespace s2c {

class ForwardingHdrStruct : public PDU {
public:


   ForwardingHdrStruct() {mName = "ForwardingHdr";}
   PDUMemberFunctions
};


}
