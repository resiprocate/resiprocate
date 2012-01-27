/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef ADD_TRANSPORT_MESSAGE_HXX
#define ADD_TRANSPORT_MESSAGE_HXX

#include "resip/stack/Message.hxx"

namespace resip
{

/**
   @internal
*/
class AddTransportMessage
{
   public:
      AddTransportMessage( TransportType protocol,
                         int port=0, 
                         IpVersion version=V4,
                         StunSetting stun=StunDisabled,
                         const Data& ipInterface = Data::Empty, 
                         const Data& sipDomainname = Data::Empty, // only used
                                                                  // for TLS
                                                                  // based stuff 
                         const Data& privateKeyPassPhrase = Data::Empty,
                         SecurityTypes::SSLType sslType = SecurityTypes::TLSv1)
         : mProtocol(protocol),
            mPort(port),
            mVersion(version),
            mStun(stun),
            mIpInterface(ipInterface),
            mSipDomainname(sipDomainname),
            mPrivateKeyPassPhrase(privateKeyPassPhrase),
            mSslType(sslType)
      {}
      
      virtual ~AddTransportMessage(){};

      virtual Message* clone() const
      {
         return new AddTransportMessage(*this);
      }
      /// output the entire message to stream
      virtual std::ostream& encode(std::ostream& strm) const
      {
      
      }
      
      /// output a brief description to stream
      virtual std::ostream& encodeBrief(std::ostream& str) const
      {
      
      }
      
   private:
      TransportType mProtocol;
      int mPort;
      IpVersion mVersion;
      StunSetting mStun;
      const Data& mIpInterface;
      const Data& mSipDomainname;
      const Data& mPrivateKeyPassPhrase;
      SecurityTypes::SSLType mSslType;
};

}

#endif

/* Copyright 2007 Estacado Systems */
