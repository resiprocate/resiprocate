#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/Message.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "rutil/WinLeakCheck.hxx"

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
