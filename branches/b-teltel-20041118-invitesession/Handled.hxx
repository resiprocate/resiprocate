#if !defined(RESIP_HANDLED_HXX)
#define RESIP_HANDLED_HXX

namespace resip
{
class HandleManager;

class Handled
{
   public:
      typedef unsigned long Id; // make this a UInt64, fix the hash

      Handled(HandleManager& ham);
      virtual ~Handled();
      
   protected:
      HandleManager& mHam;
      Handled::Id mId;
};
 
}

#endif
