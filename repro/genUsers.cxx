
#include "rutil/ResipAssert.h"
#include <iostream>

#include "repro/UserStore.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/BerkeleyDb.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cerr, Log::Err, argv[0]);
   Log::setLevel(Log::Info);
   
   const int numUsers = 100*1000;
   
   AbstractDb* db = new BerkeleyDb;

   UserStore store( *db );
   
   for ( int i=0; i<numUsers;i++)
   {
      Data user = Data("User") + Data(i);
      
      store.addUser( user,
                     "localhost", // domain
                     "localhost", // realm
                     "password", // password 
                     true,       // apply hash to password
                     "Alice W. Here", // fullName
                     "alice@example.com" /*email*/ );

      if ( i%100 == 0 )
      {
         InfoLog( << "Created " << user );
      }
   }

   return 0;
}
