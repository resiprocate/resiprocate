#include <iostream>
#ifndef WIN32
#include <unistd.h>
#endif
#include <util/Logger.hxx>
#include <sipstack/HeaderTypes.hxx>
using namespace Vocal2;
using namespace std;
#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP

void A()
{
   for(Headers::Type t = Headers::CSeq;
       t < Headers::UNKNOWN;
       ++t)
   {
      DebugLog(<< "A: "<< t << ":'"<<Headers::HeaderNames[t]<<"'");
   }
}

void B()
{
   for(int t = Headers::CSeq;
       t < Headers::UNKNOWN;
       ++t)
   {
      DebugLog(<<"B: "<< t << ":'"<<Headers::HeaderNames[t]<<"'");
   }
}
int
main()
{
   A();
   B();
}
