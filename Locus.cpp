#include "Locus.h"

using namespace p2p;
using namespace std;

std::string intToString(int inInt){
	string locIntStr;
	stringstream locStream;
	locStream << inInt;
	locStream >> locIntStr;
	return locIntStr;
}

unsigned char * Locus::initialize(std::string inString){
	if (inString.size() > 1024){
		cout << "!!! large string" << endl;
	}
	
	//Compute the Locus Value
	unsigned char locStrBuf[1024];
	memset(locStrBuf, 0, 1024);
	memcpy(locStrBuf, inString.c_str(), inString.size());
	return SHA1(locStrBuf, inString.size(), mBuffer);
}


Locus::Locus(int inInt){
	string locIntStr = intToString(inInt);
	this->mString = locIntStr;
	initialize(this->mString );
}

Locus::Locus(std::string inAddress, int inPort)
{
	string locPortStr = intToString(inPort);
	this->mString = inAddress + ":" + locPortStr;
	initialize(this->mString);
}

Locus::Locus(std::string inString)
{
	this->mString = inString;
	initialize(this->mString);
}


Locus::Locus(unsigned char * inBuffer, unsigned short int inLength){
	unsigned int locLength = static_cast<unsigned int>(inLength);
	if (locLength < LOCUS_SIZE)
		cout << "PROBLEM!!!" << endl;
	else
		memcpy(mBuffer, inBuffer, LOCUS_SIZE);
}

Locus::Locus()
{
	this->mString = "";
	memset(this->mBuffer, 0, LOCUS_SIZE);
}

Locus::~Locus()
{
}

unsigned int Locus::size(){
	return LOCUS_SIZE;
}

unsigned char * Locus::toBuffer(){
	return mBuffer;
}

string Locus::toString() const{
	BIGNUM * locBigNum = BN_bin2bn(mBuffer, LOCUS_SIZE, NULL);
	char * locShaStr = BN_bn2hex(locBigNum);

	string locString(locShaStr);
	OPENSSL_free(locShaStr);
	BN_free(locBigNum);
	return locString;
}

Locus& Locus::operator=(const std::string &inRhs) {
	this->mString = inRhs;
	initialize(this->mString );
	return *this;  // Return a reference to myself.
 }

Locus& Locus::operator=(const int &inRhs) {
	string locIntStr = intToString(inRhs);
	this->mString = locIntStr;
	initialize(this->mString );
	return *this;  // Return a reference to myself.
 }


Locus& Locus::operator=(const Locus &inRhs) {
	this->mString = inRhs.mString;
	memcpy(this->mBuffer, inRhs.mBuffer, LOCUS_SIZE);
	return *this;  // Return a reference to myself.
 }

bool Locus::operator== (const Locus &inRhs) const {
	//TODO: Fix this, it is kind of nasty, especially when using maps
	BIGNUM * locBigNum1 = BN_bin2bn(this->mBuffer, LOCUS_SIZE, NULL);
	BIGNUM * locBigNum2 = BN_bin2bn(inRhs.mBuffer, LOCUS_SIZE, NULL);
	int locResult = BN_ucmp(locBigNum1, locBigNum2);
	BN_free(locBigNum1);
	BN_free(locBigNum2);
	if (locResult == 0)
		return true;
	else
		return false;
}

bool Locus::operator< (const Locus &inRhs) const{
	//TODO: Fix this, it is kind of nasty, especially when using maps
	BIGNUM * locBigNum1 = BN_bin2bn(this->mBuffer, LOCUS_SIZE, NULL);
	BIGNUM * locBigNum2 = BN_bin2bn(inRhs.mBuffer, LOCUS_SIZE, NULL);
	int locResult = BN_ucmp(locBigNum1, locBigNum2);
	BN_free(locBigNum1);
	BN_free(locBigNum2);
	if (locResult == -1)
		return true;
	else
		return false;
}

bool Locus::operator> (const Locus &inRhs) const{
	BIGNUM * locBigNum1 = BN_bin2bn(this->mBuffer, LOCUS_SIZE, NULL);
	BIGNUM * locBigNum2 = BN_bin2bn(inRhs.mBuffer, LOCUS_SIZE, NULL);
	int locResult = BN_cmp(locBigNum1, locBigNum2);
	
	BN_free(locBigNum1);
	BN_free(locBigNum2);
	if (locResult == 1)
		return true;
	else
		return false;
}

bool Locus::operator<= (const Locus &inRhs) const{
	bool locResult = (*(this) == inRhs);
	locResult = locResult || ((*this) < inRhs);
	return locResult;
}

bool Locus::operator>= (const Locus &inRhs) const{
	bool locResult = (*(this) == inRhs);
	locResult = locResult || ((*this) >inRhs);
	return locResult;
}


void locusTest(){
	Locus w = 1;
	Locus q = 1;
	Locus e = 2;
	cout << w.toString() << endl;
	cout << q.toString() << endl;
	cout << e.toString() << endl;
	cout << (w==q) << endl;
	cout << (w<e) << endl;
	cout << (w<=e) << endl;
	cout << (w<=q) << endl;

	cout << (w>e) << endl;
	cout << (w>=e) << endl;
	cout << (w>=q) << endl;

	cout << (w == e) << endl;
	return;	
}
