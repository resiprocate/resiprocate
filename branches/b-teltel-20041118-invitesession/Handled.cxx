#include "resiprocate/dum/HandleManager.hxx"
#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


using namespace resip;

Handled::Handled(HandleManager& ham) : 
   mHam(ham),
   mId(0)
{
   mId = mHam.create(this);
   StackLog ( << "&&&&&& Handled::Handled " << mId << "this(" << this << ") " << &ham );
}

Handled::~Handled()
{
   StackLog ( << "&&&&&& ~Handled " << mId << "this(" << this << ") " << &mHam );
   mHam.remove(mId);
}

