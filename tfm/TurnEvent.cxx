#include "tfm/TurnEvent.hxx"
#include "tfm/TurnEndPoint.hxx"

TurnEvent::TurnEvent(TurnEndPoint* tua, Type type) : 
   Event(tua),
   mType(type)
{}
