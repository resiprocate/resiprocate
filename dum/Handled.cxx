#include "HandleManager.hxx"
#include "Handled.hxx"

using namespace resip;

Handled::Handled(HandleManager& ham) : 
   mHam(ham),
   mId(mHam.create(this))
{
}

Handled::~Handled()
{
   mHam.remove(mId);
}

