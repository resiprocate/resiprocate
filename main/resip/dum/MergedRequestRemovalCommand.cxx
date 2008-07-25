#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MergedRequestKey.hxx"
#include "resip/dum/MergedRequestRemovalCommand.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;


MergedRequestRemovalCommand::MergedRequestRemovalCommand(DialogUsageManager& dum,
                                                         const MergedRequestKey& key)
   : mDum(dum),
     mKey(key)
{
}

MergedRequestRemovalCommand::MergedRequestRemovalCommand(const MergedRequestRemovalCommand& from)
   : mDum(from.mDum),
     mKey(from.mKey)
{
}

void MergedRequestRemovalCommand::executeCommand()
{
   mDum.removeMergedRequest(mKey);
}

Message* MergedRequestRemovalCommand::clone() const
{
   return new MergedRequestRemovalCommand(*this);
}

EncodeStream&
MergedRequestRemovalCommand::encode(EncodeStream& strm) const
{
   return strm;
}

EncodeStream&
MergedRequestRemovalCommand::encodeBrief(EncodeStream& strm) const
{
   return strm;
}
