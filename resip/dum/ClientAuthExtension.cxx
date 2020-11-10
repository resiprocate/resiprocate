#include "resip/dum/ClientAuthExtension.hxx"

#include <utility>

using namespace resip;

std::unique_ptr<ClientAuthExtension> ClientAuthExtension::mInstance(new ClientAuthExtension());

void 
ClientAuthExtension::setInstance(std::unique_ptr<ClientAuthExtension> ext)
{
   mInstance = std::move(ext);
}


void
ClientAuthExtension::makeChallengeResponseAuth(const SipMessage& request,
                                               const Data& username,
                                               const Data& password,
                                               const Auth& challenge,
                                               const Data& cnonce,
                                               const Data& authQop,
                                               const Data& nonceCountString,
                                               Auth& auth)
{
   resip_assert(0);
}

void 
ClientAuthExtension::makeChallengeResponseAuthWithA1(const SipMessage& request,
                                                     const Data& username,
                                                     const Data& passwordHashA1,
                                                     const Auth& challenge,
                                                     const Data& cnonce,
                                                     const Data& authQop,
                                                     const Data& nonceCountString,
                                                     Auth& auth)
{
   resip_assert(0);
}

      
bool 
ClientAuthExtension::algorithmAndQopSupported(const Auth& challenge)
{
   return false;
}

