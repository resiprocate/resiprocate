#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"

using namespace resip;
using namespace std;


TargetCommand::TargetCommand(Target& target,
                             auto_ptr<Message> message)
   : mTarget(target),
     mMessage(message)
{
}

TargetCommand::TargetCommand(const TargetCommand& from)
   : mTarget(from.mTarget),
     mMessage(from.mMessage)
{
}

void TargetCommand::execute()
{
   mTarget.post(mMessage);
}

Message* TargetCommand::clone() const
{
   return new TargetCommand(*this);
}

ostream&
TargetCommand::encode(ostream& strm) const
{
   return strm;
}

ostream&
TargetCommand::encodeBrief(ostream& strm) const
{
   return strm;
}

TargetCommand::Target::~Target()
{
}
