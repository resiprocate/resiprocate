#include "resip/dum/ClientAuthExtension.hxx"

using namespace resip;

ClientAuthExtension* ClientAuthExtension::mInstance = new ClientAuthExtension();

void 
ClientAuthExtension::setInstance(ClientAuthExtension* ext)
{
   mInstance = ext;
}


Auth 
ClientAuthExtension::makeChallengeResponseAuth(SipMessage& request,
                                                    const Data& username,
                                                    const Data& password,
                                                    const Auth& challenge,
                                                    const Data& cnonce,
                                                    unsigned int& nonceCount,
                                                    Data& nonceCountString)
{
   assert(0);
   return Auth();
}

      
bool 
ClientAuthExtension::algorithmAndQopSupported(const Auth& challenge)
{
   return false;
}

   
#include "resip/dum/ClientAuthExtension.hxx"

using namespace resip;

ClientAuthExtension* ClientAuthExtension::mInstance = new ClientAuthExtension();

void 
ClientAuthExtension::setInstance(ClientAuthExtension* ext)
{
   mInstance = ext;
}


Auth 
ClientAuthExtension::makeChallengeResponseAuth(SipMessage& request,
                                                    const Data& username,
                                                    const Data& password,
                                                    const Auth& challenge,
                                                    const Data& cnonce,
                                                    unsigned int& nonceCount,
                                                    Data& nonceCountString)
{
   assert(0);
   return Auth();
}

      
bool 
ClientAuthExtension::algorithmAndQopSupported(const Auth& challenge)
{
   return false;
}

   
