#include "Util.h"
namespace p2p{

using namespace std;


void pushLabel(std::vector<unsigned int> & inLabelStack, unsigned int inLabel){
	inLabelStack.insert(inLabelStack.begin(), inLabel);
}

void pushLocus(std::vector<unsigned int> & inLabelStack, Locus inLocus)
{
	unsigned int locLabel = 1;
	unsigned char * locBuffer = inLocus.toBuffer();

	for (int i = LOCUS_PARTITIONS-1; i >= 0; i--)
	{
		memcpy(&locLabel, locBuffer + i*LABEL_SIZE, LABEL_SIZE);
		inLabelStack.insert(inLabelStack.begin(), locLabel);
	}
	locLabel = 1;
	inLabelStack.insert(inLabelStack.begin(), locLabel);
}

unsigned int popLabel(std::vector<unsigned int> & inLabelStack)
{
	if (inLabelStack.size() < 1){
		return 0;
	}
	unsigned int locLabel = inLabelStack.front();
	inLabelStack.erase(inLabelStack.begin());
	return locLabel;
}

Locus popLocus(std::vector<unsigned int> & inLabelStack)
{
	Locus dummyLocus;
	memset(dummyLocus.mBuffer, 0, LOCUS_SIZE);
	if (inLabelStack.size() < LOCUS_PARTITIONS+1){
		return dummyLocus;
	}
	
	if (inLabelStack.front() != 1){
		return dummyLocus;
	}
	
	inLabelStack.erase(inLabelStack.begin());

	unsigned char locBuffer[LOCUS_SIZE];
	
	for (int i = 0; i < LOCUS_PARTITIONS; i++)
	{
		memcpy(locBuffer + i*LABEL_SIZE, &inLabelStack[i], LABEL_SIZE);
	}
	for (int i = 0; i < LOCUS_PARTITIONS; i++)
	{
		inLabelStack.erase(inLabelStack.begin());
	}
	
	Locus locLocus(locBuffer, LOCUS_SIZE);
	return locLocus;
}

Locus topLocus(std::vector<unsigned int> & inLabelStack)
{
	Locus dummyLocus;
	memset(dummyLocus.mBuffer, 0, LOCUS_SIZE);
	if (inLabelStack.size() < LOCUS_PARTITIONS+1){
		return dummyLocus;
	}
	if (inLabelStack.front() != 1){
		return dummyLocus;
	}

	unsigned char locBuffer[LOCUS_SIZE];
	
	for (int i = 0; i < LOCUS_PARTITIONS; i++)
	{
		memcpy(locBuffer + i*LABEL_SIZE, &inLabelStack[i+1], LABEL_SIZE);
	}
	Locus locLocus(locBuffer, LOCUS_SIZE);
	return locLocus;
}

unsigned int topLabel(std::vector<unsigned int> & inLabelStack){
	if (inLabelStack.empty()){
		return 0;
	}
	unsigned int locLabel = inLabelStack.front();
	return locLabel;
}

void printLabelStack(std::vector<unsigned int> & inLabelStack){
	std::vector<unsigned int> cpyLabelStack = inLabelStack;
	int index = 0;
	while(true){
		if (cpyLabelStack.empty())
			break;
		else{
			if (cpyLabelStack.front() == 1){
				cout << "Label stack Locus[" << index++ << "]=" << popLocus(cpyLabelStack).toString() << endl;
			}
			else{
				cout << "Label stack Label [" << index++ << "]=" << popLabel(cpyLabelStack) << endl;
			}
			
		}
	}
}
}