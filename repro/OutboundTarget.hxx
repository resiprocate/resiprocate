#ifndef OutboundTarget_Include_Guard
#define OutboundTarget_Include_Guard

#include "repro/QValueTarget.hxx"

namespace repro
{
class OutboundTarget : public QValueTarget
{
   public:
      explicit OutboundTarget(const resip::ContactList& recs);
      virtual ~OutboundTarget();

      OutboundTarget* nextInstance();

      virtual OutboundTarget* clone() const;

      static bool instanceCompare(const resip::ContactInstanceRecord& lhs, 
                                    const resip::ContactInstanceRecord& rhs)
      {
         return lhs.mLastUpdated < rhs.mLastUpdated;
      }

   protected:
      resip::ContactList mList;
}; // class OutboundTarget

} // namespace repro

#endif // include guard
