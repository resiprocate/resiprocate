#include "tfm/StunEvent.hxx"
#include "tfm/StunEndPoint.hxx"

StunEvent::StunEvent(StunEndPoint* tua, Type type) : 
   Event(tua),
   mType(type)
{}
