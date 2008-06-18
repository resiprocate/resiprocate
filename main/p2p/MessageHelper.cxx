#include "p2p/MessageHelper.hxx"
#include <vector>

using namespace p2p;

std::vector<unsigned char> 
MessageHelper::convert(const resip::Data &data)
{
	std::vector<unsigned char> byteData;
	byteData.resize(data.size());
	std::copy(data.begin(), data.end(), &byteData[0]);
}

