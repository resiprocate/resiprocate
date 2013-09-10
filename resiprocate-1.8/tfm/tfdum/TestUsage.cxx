#include "DumUserAgent.hxx"
#include "TestUsage.hxx"


TestUsage::TestUsage(DumUserAgent* ua)
   : mUa(ua)
{
   mUa->add(this);
}

TestUsage::~TestUsage()
{
   mUa->remove(this);
}

DumUserAgent*
TestUsage::getDumUserAgent() const
{
   return mUa;
}
