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
      virtual ~DialogSet();
      
      DialogSetId getId();
      
      void addDialog( Dialog* );
      void removeDialog(const Dialog* );
      
      DialogIdSet getDialogs() const;

      Dialog* findDialog( const DialogId id );
      Dialog* findDialog( const Data& otherTag );
      Dialog* findDialog( SipMessage& msg );
      
      BaseCreator* getCreator();
      
   private:
      std::list<Dialog*> mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
};
 
}

#endif
