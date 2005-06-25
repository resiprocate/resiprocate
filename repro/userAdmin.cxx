
#include <cassert>
#include <iostream>

#include "repro/UserDb.hxx"
#include "repro/WebAdmin.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


void 
add( char* pUsername, char* pRealm, char* pPasswd )
{
   UserDb db;  

   Data fullName = Data::Empty;
   Data email = Data::Empty;
   
   db.addUser( Data(pUsername), 
               Data(pRealm), 
               Data(pRealm), 
               Data(pPasswd),
               fullName, 
               email );
}


void
remove( char* pAor )
{ 
   UserDb db;
   
   db.removeUser( Data(pAor) );
}


void
web(int port)
{
   assert(0);
#if 0
   UserDb db;
   
   WebAdmin webAdmin( db, port );
   
   while (1)
   {
      FdSet fdset; 
     
      webAdmin.buildFdSet(fdset);
      fdset.selectMilliSeconds( 10*1000 );

      webAdmin.process(fdset);
   }
#endif
}


void
list()
{
   UserDb db; 
   
   Data key = db.getFirstKey();
   while ( !key.empty() )
   {
      Data hash = db.getUserAuthInfo(key);
      
      clog << "Key: " << key << endl;
      clog << "  passwordHash = " << hash << endl;
      
      key = db.getNextKey();
   } 
}


void 
usage()
{
   clog << "Command line options are" << endl
        << "  -list" << endl
        << "  -add user realm password" << endl
        << "  -remove aor" << endl
        << "  -web" << endl;
}


int 
main(int argc, char* argv[])
{
   Log::initialize(Log::Cerr, Log::Err, argv[0]);
   Log::setLevel(Log::Info);

   for ( int i=1; i<argc; i++ )
   {
      if ( !strcmp(argv[i],"-list" ) )
      {
         list();
      }
      if ( !strcmp(argv[i],"-web" ) )
      {
         int port=5080;
         web(port);
      }
      else if (!strcmp(argv[i],"-add"))
      {
         i++;
         assert( i<argc );
         char* username = argv[i];

         i++;
         assert( i<argc );
         char* realm = argv[i];

         i++;
         assert( i<argc );
         char* passwd = argv[i];

         add( username, realm, passwd );
      }  
      else if (!strcmp(argv[i],"-remove"))
      {
         i++;
         assert( i<argc );
         char* aor = argv[i];
         remove( aor );
      }  
      else
      {
         usage();
         exit(1);
      }
   }

   return 0;
}
