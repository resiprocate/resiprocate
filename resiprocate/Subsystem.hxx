#ifndef Subsystem_hxx
#define Subsystem_hxx

#include <string>

namespace Vocal2
{

class Subsystem : public std::string
{
   public:
      // Add new systems below
      static const Subsystem TEST;   
      static const Subsystem NONE; // default subsystem
      static const Subsystem UTIL;
      static const Subsystem SIP;    // SIP Stack / Parser
      
   private:
      Subsystem(const std::string& str) : std::string(str) {};
};
 
}

#endif // Subsystem.hxx
