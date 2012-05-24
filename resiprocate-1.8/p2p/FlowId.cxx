#include "p2p/Event.hxx"
#include "p2p/FlowId.hxx"
#include <iostream>

namespace p2p
{

std::ostream& 
operator<<( std::ostream& strm, const FlowId& flow )
{
   strm << "Flow: " << flow.getApplication() << " " << flow.getSocket();
   return strm;
}

}
