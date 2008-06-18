#ifndef P2P_MESSAGEHELPER_HXX
#define P2P_MESSAGEHELPER_HXX

#include "rutil/Data.hxx"

namespace p2p
{
	
class MessageHelper
{
public:	
	static std::vector<unsigned char> convert(const resip::Data &data);
};

}

#endif
