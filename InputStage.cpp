#include "InputStage.h"

namespace p2p
{

using namespace std;

InputStage::InputStage()
{
	mDhtStage = NULL;
}

InputStage::~InputStage()
{
}

int InputStage::handleEvent(std::auto_ptr<p2p::EventInfo> inPtr){
	if (typeid(*inPtr) == typeid(p2p::FdCallbackEvent)){
		EventInfo * locEventInfo = inPtr.release();
		FdCallbackEvent * inCallback = (p2p::FdCallbackEvent *) (locEventInfo);
		switch(inCallback->mEventType){
		case(FdCallbackEvent::WRITE):{
			cout << "INPUT: write not implemented" << endl;
			delete inCallback;
			return FdRequestEvent::COMPLETE;
			break;
		}
		case(FdCallbackEvent::READ):{
			char buffer[10240];
			memset(buffer, 0, 10240);
			int bytes = read(inCallback->mFd, buffer, 10240);
			if (bytes > 0)
			{
				string str(buffer);
				unsigned int locBeginPos = str.find_first_not_of(" ");
				if (locBeginPos != std::string::npos)
				{
					unsigned int locEndPos = str.find_first_of(" \r\n", locBeginPos);
					string command = str.substr(locBeginPos, locEndPos - locBeginPos);
					std::vector<std::string> arguments;
					while ((locBeginPos = str.find_first_not_of(" \r\n", locEndPos)) != std::string::npos)
					{
						locEndPos = str.find_first_of(" \r\n", locBeginPos);
						if (locEndPos == std::string::npos)
							break;
						string argument = str.substr(locBeginPos, locEndPos - locBeginPos);
						arguments.push_back(argument);	
					}
					if (command == "store")
					{
						cout << "INPUT: store ";
						for (unsigned int j = 0; j < arguments.size(); j++){
							cout << ",arg[" << j << "]={" << arguments[j] << "} "; 
						}
						cout << endl;
						Locus locus;
						unsigned int expire = 120;
						string data;
						//Assign a particular locus to a storage object
						for (unsigned int j = 0; j < arguments.size();){
							if (arguments[j] == "-l")
							{
								int intLocus = atoi(arguments[j+1].c_str());
								BIGNUM * num = BN_new();
								BN_set_word(num, intLocus);
							
								memset(locus.mBuffer, 0, LOCUS_SIZE);
								unsigned int bytes = BN_num_bytes(num);
								unsigned int diff = LOCUS_SIZE - bytes;
								BN_bn2bin(num, locus.mBuffer+diff);
								BN_free(num);
								j+=2;
							}
							else if (arguments[j] == "-t")
							{
								expire = atoi(arguments[j+1].c_str());
								j+=2;
							}
							else if (arguments[j] == "-d")
							{
								data = arguments[j+1];
								j+=2;
							}
							else{
								j++;
							}
						}
						if (mDhtStage)
							mDhtStage->store(locus, StoreBlock::STRING, expire, (void *) data.c_str(), data.size());
					}
					else if (command == "print"){
						cout << "INPUT: state " << endl;
						if (mDhtStage)
							mDhtStage->printState();
					}
					else if (command == "fetch"){
						cout << "INPUT: fetch ";
						for (unsigned int j = 0; j < arguments.size(); j++){
							cout << ",arg[" << j << "]={" << arguments[j] << "} "; 
						}
						cout << endl;
						
						Locus locus;
						for (unsigned int j = 0; j < arguments.size();){
							if (arguments[j] == "-l")
							{
								int intLocus = atoi(arguments[j+1].c_str());
								BIGNUM * num = BN_new();
								BN_set_word(num, intLocus);
								
								memset(locus.mBuffer, 0, LOCUS_SIZE);
								unsigned int bytes = BN_num_bytes(num);
								unsigned int diff = LOCUS_SIZE - bytes;
								BN_bn2bin(num, locus.mBuffer+diff);
								BN_free(num);
								j+=2;
							}
						}
						
						if (mDhtStage)
							mDhtStage->fetch(locus, StoreBlock::STRING);
					}
					else if (command == "ping"){
						cout << "INPUT: ping ";
						for (unsigned int j = 0; j < arguments.size(); j++){
							cout << ",arg[" << j << "]={" << arguments[j] << "} "; 
						}
						cout << endl;
						Locus locus(arguments.front());
						if (mDhtStage)
							mDhtStage->ping(locus);
					}
					else if (command == "fetch"){
						cout << "INPUT: fetch ";
					}
					else{
						cout << "INPUT: unknown command " << command << endl;
						for (unsigned int j = 0; j < arguments.size(); j++){
							cout << ",arg[" << j << "]={" << arguments[j] << "} "; 
						}
						cout << endl;
					}
				}
				
			}
			delete inCallback;
			return FdRequestEvent::INCOMPLETE;
			break;
		}
		}
	}
	return FdRequestEvent::INCOMPLETE;
}


}
