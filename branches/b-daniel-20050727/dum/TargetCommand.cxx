#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/TargetCommand.hxx"

using namespace resip;
using namespace std;


TargetCommand::TargetCommand(Target& target,
                             auto_ptr<Message> message)
   : mTarget(target),
     mMessage(message)
{
}

void TargetCommand::execute()
{
   mTarget.post(mMessage);
}

