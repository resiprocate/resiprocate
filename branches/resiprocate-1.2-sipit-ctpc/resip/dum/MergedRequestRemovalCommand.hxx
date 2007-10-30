#if !defined(RESIP_MergedRequestRemovalCommand_HXX)
#define RESIP_MergedRequestRemovalCommand_HXX

#include "resip/dum/DumCommand.hxx"

namespace resip
{

class DialogUsageManager;

class MergedRequestRemovalCommand : public DumCommand
{
   public:
      MergedRequestRemovalCommand(DialogUsageManager& dum, const MergedRequestKey& key);
      MergedRequestRemovalCommand(const MergedRequestRemovalCommand&);
      void executeCommand();


      Message* clone() const;
      std::ostream& encode(std::ostream& strm) const;
      std::ostream& encodeBrief(std::ostream& strm) const;
      
   private:
      DialogUsageManager& mDum;
      MergedRequestKey mKey;
};

}

#endif
