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
      
      virtual std::ostream& dump(std::ostream& strm) const=0;
      
   protected:
      HandleManager& mHam;
      Handled::Id mId;
};

std::ostream& 
operator<<(std::ostream& strm, const Handled& usage);
 
}

#endif
