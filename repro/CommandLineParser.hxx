#if !defined(DUM_CommandLineParser_hxx)
#define DUM_CommandLineParser_hxx

#include <vector>
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "repro/ReproConfiguration.hxx" 

namespace resip
{

   class CommandLineParser: public ReproConfiguration
{
   public:
      CommandLineParser(int argc, char** argv);
      
};
 
}

#endif

