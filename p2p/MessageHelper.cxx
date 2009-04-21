#include "p2p/MessageHelper.hxx"
#include <vector>

using namespace p2p;

std::vector<unsigned char> 
MessageHelper::convert(const resip::Data &data)
{
	std::vector<unsigned char> byteData;
	byteData.resize(data.size());
	std::copy(data.begin(), data.end(), &byteData[0]);
	return byteData;
}

void
MessageHelper::convert(const resip::Data &data, std::vector<unsigned char> &output)
{
	output.clear();
	output.resize(data.size());
	std::copy(data.begin(), data.end(), &output[0]);
}

