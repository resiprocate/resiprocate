#ifndef Subsystem_hxx
#define Subsystem_hxx

#include <sipstack/Data.hxx>

namespace Vocal2
{

class Subsystem : public Data
{
   public:
      // Add new systems below
      static const Subsystem TEST;   
      static const Subsystem NONE; // default subsystem
      static const Subsystem UTIL;
      static const Subsystem SIP;    // SIP Stack / Parser
      
   private:
      Subsystem(const Data& str) : Data(str) {};
};
 
}

#endif // Subsystem.hxx
