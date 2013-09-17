#include "resip/stack/WsConnectionBase.hxx"

using namespace resip;

WsConnectionBase::WsConnectionBase()
   : mWsConnectionValidator(SharedPtr<WsConnectionValidator>()) // null pointer
{
}

WsConnectionBase::WsConnectionBase(SharedPtr<WsConnectionValidator> wsConnectionValidator)
   : mWsConnectionValidator(wsConnectionValidator)
{
}

WsConnectionBase::~WsConnectionBase()
{
}

SharedPtr<WsConnectionValidator> WsConnectionBase::connectionValidator() const
{
   return mWsConnectionValidator;
}
