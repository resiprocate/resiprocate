#if !defined(RESIP_CLIENTDIALOGSET_HXX)
#define RESIP_CLIENTDIALOGSET_HXX

#include "Dialog.hxx"
#include "DialogSetId.hxx"

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
      DialogSet( const SipMessage& request );
      virtual ~DialogSet();
      
      DialogSetId getId();
      
      void addDialog( Dialog* );
      void removeDialog(const Dialog* );
      
      DialogIdSet getDialogs() const;

      Dialog* findDialog( const DialogId id );
      Dialog* findDialog( const Data& otherTag );
      Dialog* findDialog( SipMessage& msg );
      
      BaseCreator* getCreator();

      void cancel(const SipMessage& cancelMsg);
      void dispatch(const SipMessage& msg);
      
      static const DialogSet Empty;

   private:
      std::list<Dialog*> mDialogs;
      BaseCreator* mCreator;
      DialogSetId mId;
};
 
}

#endif
