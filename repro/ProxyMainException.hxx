#if !defined(REPRO_PROXYMAINEXCEPTION_HXX)
#define REPRO_PROXYMAINEXCEPTION_HXX

#include "rutil/Data.hxx"
#include "rutil/BaseException.hxx"

namespace repro
{

class ProxyMainException: public resip::BaseException
{
public:
   ProxyMainException( const resip::Data& msg, const resip::Data & file, int line )
            : resip::BaseException( msg, file, line ){};
protected:
   virtual const char* name() const { return "ProxyMainException"; };
};

}

#endif