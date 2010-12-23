#include "tfcommon/TurnEvent.hxx"
#include "tfcommon/TurnEndPoint.hxx"

TurnEvent::TurnEvent(TurnEndPoint* tua, Type type) : 
   Event(tua),
   mType(type)
{}
