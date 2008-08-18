#include "repro/OutboundTarget.hxx"

namespace repro
{

OutboundTarget::OutboundTarget(const resip::ContactList& recs) :
   QValueTarget(recs.empty() ? resip::ContactInstanceRecord() : recs.front()),
   mList(recs)
{
   if(!mList.empty())
   {
      mList.pop_front();
   }
}

OutboundTarget::~OutboundTarget()
{}

bool
OutboundTarget::nextInstance()
{
   if(mList.empty())
   {
      return 0;
   }

   mList.pop_front();
   return true;
}

OutboundTarget* 
OutboundTarget::clone() const
{
   return new OutboundTarget(*this);
}

} // of namespace repro
