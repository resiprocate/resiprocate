#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include "resiprocate/sam/Dialog.hxx"

namespace resip
{

class BaseCreator;

/** @file DialogSet.hxx
 *   @todo This file is empty
 */

class DialogSet
{
   public:
      DialogSet( const BaseCreator* );
      DialogSet( Dialog* );

      voidAddDialog( Dialog* );
      voidRemoveDialog( const Dialog* );
      
      DialogIdSet getDialogs() const;
      
   private:
      std::list<Dialog> dialogs;
      BaseCreator* creator;
};
 
}

#endif
