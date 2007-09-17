#ifndef GENERICDHTSTAGE_H_
#define GENERICDHTSTAGE_H_
#include "GenericStage.h"
#include "AsyncStage.h"
#include "ForwardingStage.h"

namespace p2p
{
class AsyncStage;
class ForwardingStage;

class GenericDhtStage : public p2p::GenericStage
{
public:
	GenericDhtStage(){};
	GenericDhtStage(AsyncStage *, ForwardingStage *);
	virtual ~GenericDhtStage();
	AsyncStage * mAsyncStage;
	ForwardingStage * mForwardingStage;
	Locus mLocus;
	
};

}

#endif /*GENERICDHTSTAGE_H_*/
