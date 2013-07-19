#ifndef RECONSERVER_HXX 
#define RECONSERVER_HXX 

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Data.hxx"
#include "recon/UserAgent.hxx"
#include "rutil/ServerProcess.hxx"

using namespace resip;

class MyConversationManager;
class MyUserAgent;

namespace recon
{

class ReConServerProcess : public resip::ServerProcess
{
public:
   ReConServerProcess();
   virtual ~ReConServerProcess();

   virtual int main(int argc, char** argv);
   virtual void processCommandLine(Data& commandline, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent);
   virtual void processKeyboard(char input, MyConversationManager& myConversationManager, MyUserAgent& myUserAgent);
};

}

#endif
