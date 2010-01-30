#ifndef QOSSOCKETMANAGER_HXX
#define QOSSOCKETMANAGER_HXX

#include <rutil/Socket.hxx>

namespace reTurn {

enum EQOSServiceTypes
{
   EQOSServiceType_NoTrafficControl,      // Used to indicate that no traffic control should be applied
   EQOSServiceType_NoTraffic,             // No traffic ever flows in this direction
   EQOSServiceType_BestEffort,            // Make best effort to deliver the data in this direction
   EQOSServiceType_ControlledLoad,        // Deliver data at levels expected by best effort during conditions of no significant load
   EQOSServiceType_Guaranteed,            // As long as the data is within the flowspec, gaurentee the data is delivered on time
   EQOSServiceType_Qualitative,           // Make a better than best effort to deliver the data but exact values are not known, and allows setting of custom QOS traffic class
   EQOSServiceType_NonConforming          // The traffic on this socket is non conforming to any traffic policy in effect
};

class QosSocketManager
{
public:
   QosSocketManager() {}
   virtual ~QosSocketManager() {}

   // these methods may be called to set the QOS values on a socket
   virtual bool SocketSetDSCP(resip::Socket s, int DSCPValue, bool bUDP)
   { 
      return false;
   }

   virtual bool SocketSetServiceType(
      resip::Socket s,
      const asio::ip::udp::endpoint &tInDestinationIPAddress,
      EQOSServiceTypes eInServiceType,
      ULONG ulInBandwidthInBitsPerSecond,
      bool bUDP)
   {
      return false;
   }

   // this method must be called on any socket that was passed into one 
   // of the above methods, before that socket is closed - it may be called 
   // on sockets that were not passed into one of the above methods
   virtual void SocketClose(resip::Socket s)
   {
   }
};

}

#endif
