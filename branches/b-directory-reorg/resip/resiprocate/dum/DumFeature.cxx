#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/dum/DumFeature.hxx"
#include "resiprocate/dum/TargetCommand.hxx"

using namespace resip;


DumFeature::DumFeature(DialogUsageManager& dum, TargetCommand::Target& target) 
   : mDum(dum), mTarget(target)
{
}

DumFeature::~DumFeature()
{
}   

void DumFeature::postCommand(std::auto_ptr<Message> message)
{
   mDum.post(new TargetCommand(mTarget, message));
}
