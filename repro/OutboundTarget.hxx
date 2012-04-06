#ifndef OutboundTarget_Include_Guard
#define OutboundTarget_Include_Guard

#include "repro/QValueTarget.hxx"

namespace repro
{
class OutboundTarget : public QValueTarget
{
   public:
      OutboundTarget(const resip::Data& aor, 
                     const resip::ContactList& recs);
      virtual ~OutboundTarget();

      OutboundTarget* nextInstance();

      virtual OutboundTarget* clone() const;

      static bool instanceCompare(const resip::ContactInstanceRecord& lhs, 
                                    const resip::ContactInstanceRecord& rhs)
      {
         return lhs.mLastUpdated > rhs.mLastUpdated;
      }

      inline const resip::Data& getAor() const { return mAor;} 

   protected:
      resip::Data mAor;
      resip::ContactList mList;
}; // class OutboundTarget

} // namespace repro

#endif // include guard
