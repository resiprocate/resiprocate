#if !defined(RECON_CONFIG_HXX)
#define RECON_CONFIG_HXX 

#include <sys/ioctl.h>
#include <stdio.h>

#include <map>
#include <asio.hpp>
#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <rutil/Log.hxx>
#include <rutil/BaseException.hxx>
#include <recon/UserAgent.hxx>

namespace recon {

class ReConServerConfig : public resip::ConfigParse
{
public:

   ReConServerConfig();
   virtual ~ReConServerConfig();

   void printHelpText(int argc, char **argv);
   using resip::ConfigParse::getConfigValue;

   bool getConfigValue(const resip::Data& name, resip::NameAddr &value);
   resip::NameAddr getConfigNameAddr(const resip::Data& name, const resip::NameAddr defaultValue, bool useDefaultIfEmpty=false);

   bool getConfigValue(const resip::Data& name, ConversationProfile::SecureMediaMode &value);
   ConversationProfile::SecureMediaMode getConfigSecureMediaMode(const resip::Data& name, const ConversationProfile::SecureMediaMode defaultValue);
   bool isSecureMediaModeRequired();

   bool getConfigValue(const resip::Data& name, ConversationProfile::NatTraversalMode &value);
   ConversationProfile::NatTraversalMode getConfigNatTraversalMode(const resip::Data& name, const ConversationProfile::NatTraversalMode defaultValue);


   
private:

   bool mSecureMediaRequired;

};

} // namespace

#endif


