#ifndef INPUTSTAGE_H_
#define INPUTSTAGE_H_

#include "GenericStage.h"
#include "ChordDhtStage.h"
namespace p2p
{

class InputStage : public p2p::GenericStage
{
public:
	InputStage();
	InputStage(ChordDhtStage * inDhtStage){
		mDhtStage = inDhtStage;
	}
	virtual ~InputStage();
	int enqueueEvent(std::auto_ptr<p2p::EventInfo> ){return 0;};
	int handleEvent(std::auto_ptr<p2p::EventInfo> );
	ChordDhtStage * mDhtStage;
};

}

#endif /*INPUTSTAGE_H_*/
