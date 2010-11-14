#include "resip/dum/ClientAuthExtension.hxx"

using namespace resip;

std::auto_ptr<ClientAuthExtension> ClientAuthExtension::mInstance = std::auto_ptr<ClientAuthExtension>(new ClientAuthExtension());

void 
ClientAuthExtension::setInstance(std::auto_ptr<ClientAuthExtension> ext)
{
   mInstance = ext;
}


void
ClientAuthExtension::makeChallengeResponseAuth(SipMessage& request,
                                                    const Data& username,
                                                    const Data& password,
                                                    const Auth& challenge,
                                                    const Data& cnonce,
                                               const Data& authQop,
                                               const Data& nonceCountString,
                                               Auth& auth)
{
   assert(0);
}

      
bool 
ClientAuthExtension::algorithmAndQopSupported(const Auth& challenge)
{
   return false;
}

