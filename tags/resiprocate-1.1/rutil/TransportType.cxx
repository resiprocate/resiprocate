#include "TransportType.hxx"
#include <string>

namespace resip{

TransportType getTransportTypeFromName(const std::string & transportName)
{
    if( transportName == "UDP" )
    {
        return UDP;
    }
    if( transportName == "TCP" )
    {
        return TCP;
    }
    if( transportName == "TLS" )
    {
        return TLS;
    }
    if( transportName == "SCTP" )
    {
        return SCTP;
    }
    if( transportName == "DCCP" )
    {
        return DCCP;
    }
    if( transportName == "DTLS" )
    {
        return DTLS;
    }
    return UNKNOWN_TRANSPORT;
}

std::string getTransportNameFromType(const TransportType typeEnum)
{
    switch(typeEnum)
    {
        case UDP:  return "UDP";
        case TCP:  return "TCP";
        case TLS:  return "TLS";
        case SCTP: return "SCTP";
        case DCCP: return "DCCP";
        case DTLS: return "DTLS";
        default: return "UNKNOWN";
    }
    return "UNKNOWN_TRANSPORT";
}

resip::Data toData(const TransportType typeEnum)
{
   switch(typeEnum)
   {
      case UNKNOWN_TRANSPORT:
         return resip::Data("UNKNOWN_TRANSPORT");
      case UDP:
         return resip::Data("UDP");
      case TCP:
         return resip::Data("TCP");
      case TLS:
         return resip::Data("TLS");
      case SCTP:
         return resip::Data("SCTP");
      case DCCP:
         return resip::Data("DCCP");
      case DTLS:
         return resip::Data("DTLS");
      default:
         return resip::Data("UNKNOWN");
   }
}

TransportType toTransportType(const resip::Data & transportName)
{
   if(transportName=="UDP")
   {
      return resip::UDP;
   }
   else if(transportName=="TCP")
   {
      return resip::TCP;
   }
   else if(transportName=="TLS")
   {
      return resip::TLS;
   }
   else if(transportName=="DTLS")
   {
      return resip::DTLS;
   }
   else if(transportName=="SCTP")
   {
      return resip::SCTP;
   }
   else if(transportName=="DCCP")
   {
      return resip::DCCP;
   }
   else
   {
      return resip::UNKNOWN_TRANSPORT;      
   }
   

}

}
