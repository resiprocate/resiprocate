#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include "resiprocate/sam/Dialog.hxx"

namespace resip
{

class BaseCreator;

/** @file DialogSet.hxx
 * 
 */

class DialogSet
{
   public:
      DialogSet( const BaseCreator* );
      DialogSet( Dialog* );
      ~DialogSet();
      
      void addDialog( Dialog* );
      void removeDialog(const Dialog* );
      
      DialogIdSet getDialogs() const;
      BaseCreator* getCreator();
      
   private:
      std::list<Dialog*> mDialogs;
      BaseCreator* mCreator;
};
 
}

#endif
