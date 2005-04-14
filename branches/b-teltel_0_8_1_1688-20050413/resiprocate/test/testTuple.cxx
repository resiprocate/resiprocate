#include <iostream>
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/Connection.hxx"

using namespace resip;

int
main()
{
   typedef HashMap<Tuple, Connection*> AddrMap;
   AddrMap mMap;
   Tuple t("2000:1::203:baff:fe30:1176", 5100, V6, TCP);
   mMap[t] = 0;
   std::cerr << Inserter(mMap) << std::endl;
   
   assert(mMap.count(t) == 1);
}
