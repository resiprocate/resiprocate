#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "rutil/WinLeakCheck.hxx"

#include <utility>

using namespace resip;
using namespace std;


TargetCommand::TargetCommand(Target& target,
    unique_ptr<Message> message)
   : mTarget(target),
     mMessage(std::move(message))
{
}

void TargetCommand::executeCommand()
{
   mTarget.post(std::move(mMessage));
}

Message* TargetCommand::clone() const
{
   return new TargetCommand(mTarget, std::move(mMessage)); // !bw! this destructive clone is actually a move, which may not be right
}

EncodeStream&
TargetCommand::encode(EncodeStream& strm) const
{
   return strm;
}

EncodeStream&
TargetCommand::encodeBrief(EncodeStream& strm) const
{
   return strm;
}
