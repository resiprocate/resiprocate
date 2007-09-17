#include "GenericDhtStage.h"

namespace p2p
{

GenericDhtStage::GenericDhtStage(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
}

GenericDhtStage::~GenericDhtStage()
{
}

}
