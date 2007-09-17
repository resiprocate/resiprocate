#ifndef LOCUS_H
#define LOCUS_H

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>

//This class will wrap IDs
namespace p2p
{
const unsigned int LOCUS_SIZE = 20;

class Locus
{
public:

	Locus();
	Locus(int);
	Locus(unsigned char *, unsigned short int);
	Locus(std::string, int);
	Locus (std::string);
	unsigned char * initialize(std::string);
	~Locus();
	unsigned int size();
	unsigned char * toBuffer();
	std::string toString() const;
	Locus & operator=(const std::string &);
	Locus & operator=(const int &);
	Locus & operator=(const Locus &);
	bool operator== (const Locus &) const;
	bool operator< (const Locus &) const;
	bool operator<= (const Locus &) const;
	bool operator> (const Locus &) const;
	bool operator>= (const Locus &) const;

	std::string mString;
	unsigned char mBuffer[LOCUS_SIZE];
};

}

//typedef long long Locus;



#endif
