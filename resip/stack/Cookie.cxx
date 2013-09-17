#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Cookie.hxx"
#include "resip/stack/StringCategory.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;

//====================
// Cookie
//====================
Cookie::Cookie() :
   mName(),
   mValue()
{}

Cookie::Cookie(const Data& name, const Data& value) :
     mName(name),
     mValue(value)
{}

Cookie&
Cookie::operator=(const Cookie& rhs)
{
   if (this != &rhs)
   {
      mName = rhs.mName;
      mValue = rhs.mValue;
   }
   return *this;
}

bool
Cookie::operator==(const Cookie& other) const
{
   return name() == other.name() && value() == other.value();
}

bool Cookie::operator<(const Cookie& rhs) const
{
   return name() + value() < rhs.name() + rhs.value();
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const Cookie& c)
{
   strm << c.name() << Symbols::EQUALS[0] << c.value();
   return strm;
}

const Data&
Cookie::name() const
{
   return mName;
}

Data&
Cookie::name()
{
   return mName;
}

const Data&
Cookie::value() const
{
   return mValue;
}

Data&
Cookie::value()
{
   return mValue;
}
