#ifndef UTIL_H_
#define UTIL_H_
#include <vector>
#include "Locus.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace p2p{

const int LABEL_SIZE = sizeof(unsigned int);
const int LOCUS_PARTITIONS = LOCUS_SIZE/LABEL_SIZE;

void pushLabel(std::vector<unsigned int> & inLabelStack, unsigned int inLabel);
void pushLocus(std::vector<unsigned int> & inLabelStack, Locus inLocus);
unsigned int popLabel(std::vector<unsigned int> & inLabelStack);
Locus popLocus(std::vector<unsigned int> & inLabelStack);
Locus topLocus(std::vector<unsigned int> & inLabelStack);
unsigned int topLabel(std::vector<unsigned int> & inLabelStack);
void printLabelStack(std::vector<unsigned int> & inLabelStack);
}

#endif /*UTIL_H_*/
