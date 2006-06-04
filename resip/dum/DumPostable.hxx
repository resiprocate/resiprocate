

#if !defined(RESIP_DUM_POSTABLE_HXX)
#define RESIP_DUM_POSTABLE_HXX



// 
// ** Backwards compatibility to implement Postable interface using newer
//    Postable template.

#include "rutil/Postable.hxx"
namespace resip
{
  typedef resip::Postable<Message*> DumPostable;
};
#endif
